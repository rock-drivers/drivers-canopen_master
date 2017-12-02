#include <canopen_master/PDO.hpp>
#include <utility>
#include <canopen_master/Exceptions.hpp>
#include <canopen_master/SDO.hpp>

using namespace canopen_master;

canbus::Message canopen_master::makePDOCommunicationParametersMessage(
    uint16_t nodeId, int pdoIndex,
    PDOCommunicationParameters const& parameters)
{
    uint8_t data[4];
    toLittleEndian(data + 0, parameters.inhibit_time);
    toLittleEndian(data + 2, parameters.timer_period);
    return makeSDOInitiateDomainDownload(
        nodeId, getPDOParametersObjectId(pdoIndex), 2, data, 4);
}

bool canopen_master::isPDO(uint16_t functionCode)
{
    return (functionCode >= FUNCTION_PDO0_TRANSMIT) &&
        (functionCode < (FUNCTION_PDO0_TRANSMIT + 0x80 * 8));
}

bool canopen_master::isPDOTransmit(uint16_t functionCode)
{
    return isPDO(functionCode) && ((functionCode & 0x80) == 0x80);
}

int canopen_master::getPDOIndex(uint16_t functionCode)
{
    return (functionCode - FUNCTION_PDO0_TRANSMIT) / 0x100;
}

uint16_t canopen_master::getPDOParametersObjectId(uint8_t pdoIndex)
{
    return 0x1400 + pdoIndex;
}
uint16_t canopen_master::getPDOMappingObjectId(uint8_t pdoIndex)
{
    return 0x1A00 + pdoIndex;
}

std::vector<canbus::Message> canopen_master::makePDOMappingMessages(uint8_t nodeId, uint8_t pdoIndex, PDOMapping const& mapping)
{
    std::vector<canbus::Message> result;
    uint16_t pdoObjectId = getPDOMappingObjectId(pdoIndex);

    uint8_t mappingSize = mapping.mappings.size();
    result.push_back(makeSDOInitiateDomainDownload(nodeId, pdoObjectId, 0, &mappingSize, 1));

    for (int i = 0; i < mappingSize; ++i)
    {
        PDOMapping::MappedObject m = mapping.mappings[i];
        uint8_t buffer[4];
        buffer[0] = m.size * 8;
        buffer[1] = m.subId;
        toLittleEndian(buffer + 2, m.objectId);
        result.push_back(makeSDOInitiateDomainDownload(nodeId, pdoObjectId, i + 1, buffer, 4));
    }
    return result;
}
