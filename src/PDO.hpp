#ifndef CANOPEN_MASTER_PDO_HPP
#define CANOPEN_MASTER_PDO_HPP

#include <cstdint>
#include <canmessage.hh>
#include <canopen_master/PDOMapping.hpp>
#include <canopen_master/PDOCommunicationParameters.hpp>

namespace canopen_master
{
    canbus::Message makePDOCommunicationParametersMessage(
        uint16_t nodeId, int pdoIndex,
        PDOCommunicationParameters const& parameters);

    int getPDOIndex(uint16_t functionCode);
    bool isPDO(uint16_t functionCode);
    bool isPDOTransmit(uint16_t functionCode);
    uint16_t getPDOParametersObjectId(uint8_t pdoIndex);
    uint16_t getPDOMappingObjectId(uint8_t pdoIndex);
    std::vector<canbus::Message> makePDOMappingMessages(uint8_t nodeId, uint8_t pdoIndex, PDOMapping const& mapping);
}

#endif
