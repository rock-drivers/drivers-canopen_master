#ifndef CANOPEN_MASTER_SDO_HPP
#define CANOPEN_MASTER_SDO_HPP

#include <canopen_master/Frame.hpp>
#include <tuple>

namespace canopen_master
{
    enum SDO_COMMANDS
    {
        SDO_INITIATE_DOMAIN_DOWNLOAD = 1,
        SDO_INITIATE_DOMAIN_DOWNLOAD_REPLY = 3,
        SDO_DOWNLOAD_DOMAIN_SEGMENT = 0,
        SDO_DOWNLOAD_DOMAIN_SEGMENT_REPLY = 1,
        SDO_INITIATE_DOMAIN_UPLOAD = 2,
        SDO_INITIATE_DOMAIN_UPLOAD_REPLY = 2,
        SDO_UPLOAD_DOMAIN_SEGMENT = 3,
        SDO_UPLOAD_DOMAIN_SEGMENT_REPLY = 0,
        SDO_ABORT_DOMAIN_TRANSFER = 4
    };

    struct SDOCommand
    {
        SDO_COMMANDS command;
        bool toggle_bit;
        bool expedited_transfer;
        uint32_t size;
    };

    uint16_t getSDOObjectID(canbus::Message const& msg);
    uint8_t getSDOObjectSubID(canbus::Message const& msg);
    canbus::Message makeSDOInitiateDomainUpload(uint8_t nodeId, uint16_t objectIndex, uint8_t objectSubindex);
    SDOCommand getSDOCommand(canbus::Message const& msg);
    canbus::Message makeSDOInitiateDomainDownload(uint8_t nodeId, uint16_t objectIndex, uint8_t objectSubindex,
        uint8_t const* data, uint32_t size
    );

    struct DeviceObject
    {
        uint16_t id;
        uint8_t  subId;
        uint8_t raw[8];
    };
};

#endif
