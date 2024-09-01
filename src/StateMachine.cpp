#include <algorithm>
#include <canopen_master/Emergency.hpp>
#include <canopen_master/Exceptions.hpp>
#include <canopen_master/Frame.hpp>
#include <canopen_master/NMT.hpp>
#include <canopen_master/Objects.hpp>
#include <canopen_master/PDO.hpp>
#include <canopen_master/SDO.hpp>
#include <canopen_master/StateMachine.hpp>
#include <cstring>
#include <iostream>

using namespace canopen_master;

StateMachine::Dictionary::iterator StateMachine::declareInternal(uint16_t objectId,
    uint8_t subId,
    uint8_t size,
    bool knownSize)
{
    ObjectValue value;
    value.objectId = objectId;
    value.subId = subId;
    value.size = size;
    value.knownSize = knownSize;
    return dictionary.insert(std::make_pair(ObjectIdentifier(objectId, subId), value))
        .first;
}

StateMachine::StateMachine(uint8_t nodeId, bool useUnknownSizes)
    : nodeId(nodeId)
    , useUnknownSizes(useUnknownSizes)
{
    rpdoMappings.resize(MAX_PDO);
    tpdoMappings.resize(MAX_PDO);
}

void StateMachine::setQuirks(uint64_t value)
{
    this->quirks = value;
}

bool StateMachine::hasState() const
{
    return !lastStateUpdate.isNull();
}

base::Time StateMachine::getLastStateUpdate() const
{
    return lastStateUpdate;
}

base::Time StateMachine::getLastMessageTime() const
{
    return lastMessageTime;
}

NODE_STATE StateMachine::getState() const
{
    if (lastStateUpdate.isNull())
        throw std::runtime_error("never received a state update from the node");
    return state;
}

canbus::Message StateMachine::queryState() const
{
    return makeNMTNodeGuard(nodeId);
}

canbus::Message StateMachine::queryStateTransition(NODE_STATE_TRANSITION transition) const
{
    return makeModuleControlCommand(transition, nodeId);
}

uint8_t StateMachine::getNodeID() const
{
    return nodeId;
}

uint8_t StateMachine::getUseUnknownSizes() const
{
    return useUnknownSizes;
}

void StateMachine::setUseUnknownSizes(bool toggle)
{
    useUnknownSizes = toggle;
}

StateMachine::Update StateMachine::process(canbus::Message const& msg)
{
    if (canopen_master::getNodeID(msg) == nodeId)
        lastMessageTime = msg.time;
    else
        return Update(PROCESSED_NOT_FOR_ME);

    uint16_t functionCode = getFunctionCode(msg);
    if (functionCode == FUNCTION_EMERGENCY)
        return processEmergency(msg);
    if (functionCode == FUNCTION_NMT_HEARTBEAT)
        return processHeartbeat(msg);
    if (functionCode == FUNCTION_SDO_TRANSMIT)
        return processSDOReceive(msg);
    if (isPDOTransmit(functionCode)) {
        int pdoIndex = getPDOIndex(functionCode);
        return processPDOReceive(pdoIndex, msg);
    }
    return Update();
}

StateMachine::Update StateMachine::processEmergency(canbus::Message const& msg)
{
    Emergency em = parseEmergencyMessage(msg);
    if (em.code >> 8 == 0) // "No error" ????
        return Update(PROCESSED_EMERGENCY_NO_ERROR);

    uint16_t objectId = ErrorRegister::OBJECT_ID;
    uint16_t objectSubId = ErrorRegister::OBJECT_SUB_ID;

    set<uint8_t>(objectId, objectSubId, msg.data[2]);
    throw EmergencyMessageReceived(em);
}

StateMachine::Update StateMachine::processHeartbeat(canbus::Message const& msg)
{
    state = static_cast<NODE_STATE>(msg.data[0]);
    lastStateUpdate = msg.time;
    return Update(PROCESSED_HEARTBEAT);
}

StateMachine::Update StateMachine::processPDOReceive(int pdoIndex,
    canbus::Message const& msg)
{
    if (tpdoMappings.size() < pdoIndex + 1u)
        return Update(PROCESSED_PDO_UNEXPECTED);
    PDOMapping const& mapping = tpdoMappings[pdoIndex];

    Update update(PROCESSED_PDO);
    int offset = 0;
    for (const auto m : mapping.mappings) {
        setObjectValue(m.objectId, m.subId, msg.time, msg.data + offset, m.size);
        offset += m.size;
        update.addUpdate(m.objectId, m.subId);
    }
    if (!update.hasUpdatedObjects()) {
        return Update(PROCESSED_PDO_UNEXPECTED);
    }
    else {
        return update;
    }
}

StateMachine::Update StateMachine::processSDOReceive(canbus::Message const& msg)
{
    SDOCommand cmd = getSDOCommand(msg);
    if (cmd.command == SDO_ABORT_DOMAIN_TRANSFER) {
        parseSDODomainTransferAbort(msg);
        // never returns
        return Update();
    }
    if (cmd.command == SDO_INITIATE_DOMAIN_UPLOAD_REPLY) {
        if (!cmd.expedited_transfer) {
            std::cerr << "can_master::StateMachine nodeId=" << nodeId
                      << " ignored non-expedited SDO transfer" << std::endl;
            return Update();
        }
        uint16_t objectId = getSDOObjectID(msg);
        uint8_t subId = getSDOObjectSubID(msg);
        if (msg.time.isNull()) {
            throw ProtocolError("received CAN message with zero timestamp");
        }
        if (cmd.size == 0) {
            if (has(objectId, subId)) {
                cmd.size = sizeOf(objectId, subId);
            }
            else {
                cmd.size = 4;
                declareInternal(objectId, subId, cmd.size, false);
            }
        }
        setObjectValue(objectId, subId, msg.time, msg.data + 4, cmd.size);
        return Update(PROCESSED_SDO, objectId, subId);
    }
    else if (cmd.command == SDO_INITIATE_DOMAIN_DOWNLOAD_REPLY) {
        uint16_t objectId = getSDOObjectID(msg);
        uint8_t subId = getSDOObjectSubID(msg);
        return Update(PROCESSED_SDO_INITIATE_DOWNLOAD, objectId, subId);
    }
    else {
        std::cerr << "can_master::StateMachine nodeId=" << nodeId
                  << " ignored SDO command " << cmd.command << std::endl;
        return Update(PROCESSED_SDO_IGNORED_COMMAND);
    }
    return Update(PROCESSED_SDO_UNKNOWN_COMMAND);
}

void StateMachine::setObjectValue(uint16_t objectId,
    uint8_t subId,
    base::Time const& time,
    uint8_t const* data,
    uint32_t dataSize)
{
    auto dictionary_it = dictionary.find(ObjectIdentifier(objectId, subId));
    if (dictionary_it == dictionary.end())
        dictionary_it = declareInternal(objectId, subId, dataSize, true);

    ObjectValue& value = dictionary_it->second;

    if ((value.size != dataSize) && value.knownSize)
        throw ProtocolError("unexpected object size in dictionary");

    if (time.isNull()) {
        throw std::invalid_argument(
            "attempting to set an object with a zero update time");
    }

    value.lastUpdate = time;
    std::copy(data, data + dataSize, value.data);
}

canbus::Message StateMachine::upload(uint16_t objectId, uint8_t subId) const
{
    return makeSDOInitiateDomainUpload(nodeId, objectId, subId);
}

void StateMachine::declare(uint16_t objectId, uint8_t subId, uint32_t size)
{
    declareInternal(objectId, subId, size, true);
}

bool StateMachine::has(uint16_t objectId, uint8_t subId) const
{
    return sizeOf(objectId, subId) != 0;
}

uint32_t StateMachine::sizeOf(uint16_t objectId, uint8_t subId) const
{
    auto it = dictionary.find(ObjectIdentifier(objectId, subId));
    if (it == dictionary.end())
        return 0;
    else
        return it->second.size;
}

base::Time StateMachine::timestamp(uint16_t objectId, uint8_t subId) const
{
    auto it = dictionary.find(ObjectIdentifier(objectId, subId));
    if (it == dictionary.end())
        return base::Time();
    else
        return it->second.lastUpdate;
}

canbus::Message StateMachine::sync()
{
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x80;
    msg.size = 0;
    return msg;
}

canbus::Message StateMachine::download(uint16_t objectId,
    uint8_t subId,
    uint8_t const* data,
    uint32_t size) const
{
    uint32_t knownSize = sizeOf(objectId, subId);
    if (knownSize && knownSize != size)
        throw ObjectSizeMismatch(
            "attempting to write to a SDO object that has a mismatched size");

    return makeSDOInitiateDomainDownload(nodeId,
        objectId,
        subId,
        data,
        size,
        !useUnknownSizes);
}

uint32_t StateMachine::get(uint16_t objectId,
    uint16_t subId,
    uint8_t* data,
    uint32_t bufferSize) const
{
    auto it = dictionary.find(ObjectIdentifier(objectId, subId));
    if (it == dictionary.end())
        return 0;
    if (it->second.lastUpdate.isNull())
        return 0;
    uint32_t actualSize = it->second.size;
    if (actualSize > bufferSize)
        throw BufferSizeTooSmall("buffer size too small in get()");
    std::memcpy(data, it->second.data, actualSize);
    return actualSize;
}

canbus::Message StateMachine::getRPDOMessage(unsigned int pdoIndex)
{
    if (rpdoMappings.size() < pdoIndex)
        throw std::invalid_argument("no RPDO declared with this index");

    auto mapping = rpdoMappings[pdoIndex];

    canbus::Message msg;
    msg.can_id = getPDODefaultCOBID(false, pdoIndex, nodeId);

    int offset = 0;
    for (const auto m : mapping.mappings) {
        get(m.objectId, m.subId, msg.data + offset, m.size);
        offset += m.size;
    }
    msg.size = offset;
    return msg;
}

void StateMachine::validatePDOMapping(PDOMapping const& mapping) const
{
    for (const auto m : mapping.mappings) {
        uint32_t knownSize = sizeOf(m.objectId, m.subId);
        if (knownSize && m.size != knownSize)
            throw ObjectSizeMismatch(
                "size mismatch between the PDO mapping and the registered entry");
    }
}

canbus::Message StateMachine::disablePDO(bool transmit,
    uint8_t pdoIndex,
    uint32_t cob_id) const
{
    return disablePDOMessage(transmit,
        nodeId,
        pdoIndex,
        cob_id,
        quirks & PDO_COBID_MESSAGE_RESERVED_BIT_QUIRK);
}

std::vector<canbus::Message> StateMachine::configurePDO(bool transmit,
    uint8_t pdoIndex,
    PDOCommunicationParameters const& parameters,
    PDOMapping const& mapping) const
{
    return makePDOConfigurationMessages(transmit,
        nodeId,
        pdoIndex,
        parameters,
        mapping,
        quirks & PDO_COBID_MESSAGE_RESERVED_BIT_QUIRK);
}

std::vector<canbus::Message> StateMachine::configurePDOMapping(bool transmit,
    uint8_t pdoIndex,
    PDOMapping const& mapping) const
{
    validatePDOMapping(mapping);
    return makePDOMappingMessages(transmit, nodeId, pdoIndex, mapping);
}

void StateMachine::declareTPDOMapping(uint8_t pdoIndex, PDOMapping const& mapping)
{
    declarePDOMapping(pdoIndex, mapping, tpdoMappings);
}

void StateMachine::declareRPDOMapping(uint8_t pdoIndex, PDOMapping const& mapping)
{
    declarePDOMapping(pdoIndex, mapping, rpdoMappings);
}

void StateMachine::declarePDOMapping(uint8_t pdoIndex,
    PDOMapping const& mapping,
    std::vector<PDOMapping>& mappings)
{
    validatePDOMapping(mapping);

    for (const auto m : mapping.mappings)
        declare(m.objectId, m.subId, m.size);

    if (pdoIndex + 1u > mappings.size())
        mappings.resize(pdoIndex + 1);
    mappings[pdoIndex] = mapping;
}

std::vector<canbus::Message> StateMachine::configurePDOParameters(bool transmit,
    uint8_t pdoIndex,
    PDOCommunicationParameters const& parameters) const
{
    return makePDOCommunicationParametersMessages(transmit, nodeId, pdoIndex, parameters);
}

StateMachine::Update::Update()
    : mode(PROCESSED_IGNORED_MESSAGE)
    , update_count(0)
{
}
StateMachine::Update::Update(UPDATE_EVENT mode)
    : mode(mode)
    , update_count(0)
{
}
StateMachine::Update::Update(UPDATE_EVENT mode, uint16_t objectId, uint8_t subId)
    : StateMachine::Update::Update(mode)
{
    addUpdate(objectId, subId);
}

void StateMachine::Update::addUpdate(uint16_t objectId, int8_t subId)
{
    updated[update_count] = ObjectIdentifier(objectId, subId);
    update_count++;
}

bool StateMachine::Update::hasUpdatedObjects() const
{
    return update_count != 0;
}

bool StateMachine::Update::hasUpdatedObject(uint16_t objectId, int8_t subId) const
{
    return find(updated, updated + update_count, ObjectIdentifier(objectId, subId)) !=
           updated + update_count;
}

bool StateMachine::Update::operator==(Update const& other) const
{
    if (mode != other.mode || update_count != other.update_count)
        return false;
    return std::equal(updated, updated + update_count, other.updated);
}
