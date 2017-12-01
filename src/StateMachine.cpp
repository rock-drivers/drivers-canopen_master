#include <canopen_master/StateMachine.hpp>
#include <canopen_master/Frame.hpp>
#include <canopen_master/SDO.hpp>
#include <canopen_master/PDO.hpp>
#include <canopen_master/NMT.hpp>
#include <canopen_master/Exceptions.hpp>
#include <canopen_master/Emergency.hpp>
#include <iostream>
#include <cstring>

using namespace canopen_master;

StateMachine::Dictionary::iterator StateMachine::declareInternal(uint16_t objectId, uint8_t subId, uint8_t size)
{
    ObjectValue value;
    value.objectId = objectId;
    value.subId = subId;
    value.data.resize(size);
    return dictionary.insert(
        std::make_pair(ObjectIdentifier(objectId, subId), value)
    ).first;
}

StateMachine::StateMachine(uint8_t nodeId)
    : nodeId(nodeId)
{
    pdoMappings.resize(MAX_PDO);
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

uint8_t StateMachine::getNodeID() const
{
    return nodeId;
}

bool StateMachine::process(canbus::Message const& msg)
{
    if (canopen_master::getNodeID(msg) == nodeId)
        lastMessageTime = msg.time;
    else
        return false;

    uint16_t functionCode = getFunctionCode(msg);
    if (functionCode == FUNCTION_EMERGENCY)
        return processEmergency(msg);
    if (functionCode == FUNCTION_NMT_HEARTBEAT)
        return processHeartbeat(msg);
    if (functionCode == FUNCTION_SDO_RECEIVE)
        return processSDOReceive(msg);
    if (isPDOReceive(functionCode))
    {
        int pdoIndex = getPDOIndex(functionCode);
        return processPDOReceive(pdoIndex, msg);
    }
    return false;
}

bool StateMachine::processEmergency(canbus::Message const& msg)
{
    Emergency em = parseEmergencyMessage(msg);
    if (em.code >> 8 == 0) // "No error" ????
        return true;

    throw EmergencyMessageReceived(em.code);
}

bool StateMachine::processHeartbeat(canbus::Message const& msg)
{
    state = static_cast<NODE_STATE>(msg.data[0]);
    lastStateUpdate = msg.time;
    return true;
}

bool StateMachine::processPDOReceive(int pdoIndex, canbus::Message const& msg)
{
    if (pdoMappings.size() < pdoIndex + 1u)
        return false;
    PDOMapping const& mapping = pdoMappings[pdoIndex];

    int offset = 0;
    for (const auto m : mapping.mappings)
    {
        setObjectValue(m.objectId, m.subId, msg.time, msg.data + offset, m.size);
        offset += m.size;
    }
    return !mapping.mappings.empty();
}

bool StateMachine::processSDOReceive(canbus::Message const& msg)
{
    SDOCommand cmd = getSDOCommand(msg);
    if (cmd.command == SDO_ABORT_DOMAIN_TRANSFER)
    {
        parseSDODomainTransferAbort(msg);
        // never returns
        return true;
    }
    if (cmd.command == SDO_INITIATE_DOMAIN_UPLOAD_REPLY)
    {
        if (!cmd.expedited_transfer)
        {
            std::cerr << "can_master::StateMachine nodeId=" << nodeId << " ignored non-expedited SDO transfer" << std::endl;
            return false;
        }
        uint16_t objectId = getSDOObjectID(msg);
        uint8_t  subId    = getSDOObjectSubID(msg);
        setObjectValue(objectId, subId, msg.time, msg.data + 4, cmd.size);
        return true;
    }
    else if (cmd.command == SDO_INITIATE_DOMAIN_DOWNLOAD_REPLY)
    {
        return true;
    }
    else
    {
        std::cerr << "can_master::StateMachine nodeId=" << nodeId << " ignored SDO command " << cmd.command << std::endl;
        return false;
    }
    return false;
}

void StateMachine::setObjectValue(uint16_t objectId, uint8_t subId, base::Time const& time, uint8_t const* data, uint32_t dataSize)
{
    auto dictionary_it = dictionary.find(ObjectIdentifier(objectId, subId));
    if (dictionary_it == dictionary.end())
        dictionary_it = declareInternal(objectId, subId, dataSize);

    ObjectValue& value = dictionary_it->second;

    if (value.data.size() != dataSize)
        throw ProtocolError("unexpected object size in dictionary");

    value.lastUpdate = time;
    std::copy(data, data + dataSize, value.data.data());
}

canbus::Message StateMachine::upload(uint16_t objectId, uint8_t subId) const
{
    return makeSDOInitiateDomainUpload(nodeId, objectId, subId);
}

void StateMachine::declare(uint16_t objectId, uint8_t subId, uint32_t size)
{
    declareInternal(objectId, subId, size);
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
        return it->second.data.size();
}

base::Time StateMachine::timestamp(uint16_t objectId, uint8_t subId) const
{
    auto it = dictionary.find(ObjectIdentifier(objectId, subId));
    if (it == dictionary.end())
        return base::Time();
    else return it->second.lastUpdate;
}

canbus::Message StateMachine::sync() const
{
    canbus::Message msg;
    msg.can_id = 0x100;
    return msg;
}

canbus::Message StateMachine::download(uint16_t objectId, uint8_t subId, uint8_t const* data, uint32_t size) const
{
    uint32_t knownSize = sizeOf(objectId, subId);
    if (knownSize && knownSize != size)
        throw ObjectSizeMismatch("attempting to write to a SDO object that has a mismatched size");

    return makeSDOInitiateDomainDownload(nodeId, objectId, subId, data, size);
}

uint32_t StateMachine::get(uint16_t objectId, uint16_t subId, uint8_t* data, uint32_t bufferSize) const
{
    auto it = dictionary.find(ObjectIdentifier(objectId, subId));
    if (it == dictionary.end())
        return 0;
    if (it->second.lastUpdate.isNull())
        return 0;
    uint32_t actualSize = it->second.data.size();
    if (actualSize > bufferSize)
        throw BufferSizeTooSmall("buffer size too small in get()");
    std::memcpy(data, it->second.data.data(), actualSize);
    return actualSize;
}

void StateMachine::validatePDOMapping(PDOMapping const& mapping)
{
    for (const auto m : mapping.mappings)
    {
        uint32_t knownSize = sizeOf(m.objectId, m.subId);
        if (knownSize && m.size != knownSize)
            throw ObjectSizeMismatch("size mismatch between the PDO mapping and the registered entry");
    }
}

std::vector<canbus::Message> StateMachine::configurePDOMapping(uint8_t pdoIndex, PDOMapping const& mapping)
{
    validatePDOMapping(mapping);
    return makePDOMappingMessages(nodeId, pdoIndex, mapping);
}

void StateMachine::declarePDOMapping(uint8_t pdoIndex, PDOMapping const& mapping)
{
    validatePDOMapping(mapping);

    for (const auto m : mapping.mappings)
        declare(m.objectId, m.subId, m.size);

    if (pdoIndex + 1u > pdoMappings.size())
        pdoMappings.resize(pdoIndex + 1);
    pdoMappings[pdoIndex] = mapping;
}

canbus::Message StateMachine::configurePDOParameters(uint8_t pdoIndex, PDOCommunicationParameters const& parameters)
{
    return makePDOCommunicationParametersMessage(nodeId, pdoIndex, parameters);
}
