#ifndef CANOPEN_MASTER_PDO_HPP
#define CANOPEN_MASTER_PDO_HPP

#include <cstdint>
#include <canmessage.hh>
#include <canopen_master/PDOMapping.hpp>
#include <canopen_master/PDOCommunicationParameters.hpp>

namespace canopen_master
{
    int getPDOIndex(uint16_t functionCode);
    bool isPDO(uint16_t functionCode);
    bool isPDOTransmit(uint16_t functionCode);
    uint16_t getPDODefaultCOBID(bool transmit, int pdoIndex, uint16_t nodeId);
    uint16_t getPDOParametersObjectId(bool transmit, uint8_t pdoIndex);
    uint16_t getPDOMappingObjectId(bool transmit, uint8_t pdoIndex);

    std::vector<canbus::Message> makePDOCommunicationParametersMessages(
        bool transmit, uint16_t nodeId, int pdoIndex,
        PDOCommunicationParameters const& parameters);
    std::vector<canbus::Message> makePDOMappingMessages(
        bool transmit, uint8_t nodeId, uint8_t pdoIndex,
        PDOMapping const& mapping);
    std::vector<canbus::Message> makePDOConfigurationMessages(
        bool transmit, uint16_t nodeId, int pdoIndex,
        PDOCommunicationParameters const& parameters,
        PDOMapping const& mappings);
}

#endif
