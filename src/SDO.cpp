#include <canopen_master/SDO.hpp>
#include <canopen_master/Frame.hpp>
#include <canopen_master/Exceptions.hpp>
#include <cstring>

using namespace canopen_master;

canbus::Message canopen_master::makeSDOInitiateDomainUpload(uint8_t nodeId, uint16_t objectIndex, uint8_t objectSubindex)
{
    canbus::Message msg;
    msg.can_id = FUNCTION_SDO_RECEIVE + nodeId;
    msg.size = 8;
    std::memset(msg.data, 0, sizeof(msg.data));
    msg.data[0] = 0x40;
    msg.data[1] = (objectIndex >> 0) & 0xFF;
    msg.data[2] = (objectIndex >> 8) & 0xFF;
    msg.data[3] = objectSubindex;
    return msg;
}

canbus::Message canopen_master::makeSDOInitiateDomainDownload(uint8_t nodeId, uint16_t objectIndex, uint8_t objectSubindex,
    uint8_t const* data, uint32_t size)
{
    if (size > 4)
        throw Unsupported("canopen_master: this library does not support non-expedited transfers");

    canbus::Message msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.can_id = FUNCTION_SDO_RECEIVE + nodeId;
    msg.size = 8;
    // Immediate transfer with size
    msg.data[0] = 0x23 | ((4-size) << 2);
    toLittleEndian<uint16_t>(msg.data + 1, objectIndex);
    toLittleEndian<uint8_t>(msg.data + 3, objectSubindex);
    std::memcpy(msg.data + 4, data, size);
    return msg;
}


uint16_t canopen_master::getSDOObjectID(canbus::Message const& msg)
{
    return fromLittleEndian<uint16_t>(msg.data + 1);
}
uint8_t canopen_master::getSDOObjectSubID(canbus::Message const& msg)
{
    return fromLittleEndian<uint8_t>(msg.data + 3);
}

SDOCommand canopen_master::getSDOCommand(canbus::Message const& msg)
{
    SDOCommand cmd;
    uint8_t cmd_byte = msg.data[0];

    cmd.command = static_cast<SDO_COMMANDS>(cmd_byte >> 5);
    cmd.expedited_transfer = (cmd_byte & 2) != 0;
    cmd.toggle_bit = (cmd_byte & 16) != 0;

    bool s = (cmd_byte & 1) != 0;
    if (!s)
    {
        cmd.size = 0;
    }
    else if (cmd.expedited_transfer)
    {
        cmd.size = 4 - ((cmd_byte >> 2) & 0x3);
    }
    else
    {
        cmd.size = fromLittleEndian<uint32_t>(msg.data + 4);
    }
    return cmd;
}

void canopen_master::parseSDODomainTransferAbort(canbus::Message const& msg)
{
    uint16_t objectId = fromLittleEndian<uint16_t>(msg.data + 1);
    uint8_t  subId    = fromLittleEndian<uint8_t>(msg.data + 3);
    uint32_t rawCode  = fromLittleEndian<uint32_t>(msg.data + 4);
    throw SDODomainTransferAborted(objectId, subId, rawCode);
}
