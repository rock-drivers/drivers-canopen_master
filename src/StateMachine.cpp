#include <canopen_master/StateMachine.hpp>
#include <canopen_master/Frame.hpp>
#include <canopen_master/SDO.hpp>
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

bool StateMachine::processSDOReceive(canbus::Message const& msg)
{
    SDOCommand cmd = getSDOCommand(msg);
    if (cmd.command == SDO_INITIATE_DOMAIN_UPLOAD_REPLY)
    {
        if (!cmd.expedited_transfer)
        {
            std::cerr << "can_master::StateMachine nodeId=" << nodeId << " ignored non-expedited SDO transfer" << std::endl;
            return false;
        }
        uint16_t objectId = getSDOObjectID(msg);
        uint8_t  subId    = getSDOObjectSubID(msg);
        auto dictionary_it = dictionary.find(ObjectIdentifier(objectId, subId));
        if (dictionary_it == dictionary.end())
            dictionary_it = declareInternal(objectId, subId, cmd.size);

        ObjectValue& value = dictionary_it->second;

        if (value.data.size() != cmd.size)
            throw ProtocolError("unexpected object size in dictionary");

        value.lastUpdate = msg.time;
        std::copy(msg.data + 4, msg.data + 4 + cmd.size,
            value.data.data());
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
    return dictionary.find(ObjectIdentifier(objectId, subId)) != dictionary.end();
}

base::Time StateMachine::timestamp(uint16_t objectId, uint8_t subId) const
{
    auto it = dictionary.find(ObjectIdentifier(objectId, subId));
    if (it == dictionary.end())
        return base::Time();
    else return it->second.lastUpdate;
}

canbus::Message StateMachine::download(uint16_t objectId, uint8_t subId, uint8_t const* data, uint32_t size) const
{
    auto it = dictionary.find(ObjectIdentifier(objectId, subId));
    if (it != dictionary.end())
    {
        if (it->second.data.size() != size)
            throw InvalidObjectType("attempting to write to a SDO object that has a mismatched size");
    }

    return makeSDOInitiateDomainDownload(nodeId, objectId, subId, data, size);
}

uint32_t StateMachine::get(uint16_t objectId, uint16_t subId, uint8_t* data, uint32_t bufferSize) const
{
    auto it = dictionary.find(ObjectIdentifier(objectId, subId));
    if (it == dictionary.end())
        return 0;
    uint32_t actualSize = it->second.data.size();
    if (actualSize > bufferSize)
        throw BufferSizeTooSmall("buffer size too small in get()");
    std::memcpy(data, it->second.data.data(), actualSize);
    return actualSize;
}
