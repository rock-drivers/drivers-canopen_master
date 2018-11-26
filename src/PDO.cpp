#include <canopen_master/PDO.hpp>
#include <utility>
#include <canopen_master/Exceptions.hpp>
#include <canopen_master/SDO.hpp>

using namespace std;
using namespace canopen_master;

canbus::Message canopen_master::disablePDOMessage(
        bool transmit, uint16_t nodeId, int pdoIndex, uint32_t cob_id)
{
    uint32_t sdoObjId = getPDOParametersObjectId(transmit, pdoIndex);
    if (!cob_id)
        cob_id = getPDODefaultCOBID(transmit, pdoIndex, nodeId);

    // Set up COB-ID
    uint8_t data[4];
    toLittleEndian(data, cob_id);
    auto message = makeSDOInitiateDomainDownload(nodeId, sdoObjId, 1, data, 4);
    message.data[7] |= 0x80;
    return message;
}

vector<canbus::Message> canopen_master::makePDOConfigurationMessages(
        bool transmit, uint16_t nodeId, int pdoIndex,
        PDOCommunicationParameters const& parameters,
        PDOMapping const& mappings)
{
    auto messages = makePDOCommunicationParametersMessages(
        transmit, nodeId, pdoIndex, parameters);
    auto pdoCOB_IDSetting = messages[0];
    // Disable the PDO by setting bit 31. We will reset this
    // later.
    messages[0].data[7] |= 0x80;

    auto mappingMessages = makePDOMappingMessages(
        transmit, nodeId, pdoIndex, mappings);
    messages.insert(
        messages.end(),
        mappingMessages.begin(), mappingMessages.end());

    // Now re-enable the message
    messages.push_back(pdoCOB_IDSetting);
    return messages;
}

vector<canbus::Message> canopen_master::makePDOCommunicationParametersMessages(
    bool transmit, uint16_t nodeId, int pdoIndex,
    PDOCommunicationParameters const& parameters)
{
    uint32_t sdoObjId = getPDOParametersObjectId(transmit, pdoIndex);
    uint32_t cob_id = parameters.cob_id;
    if (!cob_id)
        cob_id = getPDODefaultCOBID(transmit, pdoIndex, nodeId);

    vector<canbus::Message> messages;

    // Set up COB-ID
    uint8_t data[4];
    toLittleEndian(data, cob_id);
    messages.push_back(makeSDOInitiateDomainDownload(nodeId, sdoObjId, 1, data, 4));

    // Set up mode
    data[0] = 0;
    switch(parameters.transmission_mode)
    {
        case PDO_SYNCHRONOUS:
            if (parameters.sync_period > 251)
                throw std::invalid_argument("invalid sync_period in PDO_SYNCHRONOUS mode, must be between 0 and 251");
            data[0] = parameters.sync_period;
            break;
        case PDO_SYNCHRONOUS_RTR_ONLY:
            data[0] = 252;
            break;
        case PDO_ASYNCHRONOUS_RTR_ONLY:
            data[0] = 253;
            break;
        case PDO_ASYNCHRONOUS:
            data[0] = 254;
            break;
    }
    messages.push_back(makeSDOInitiateDomainDownload(nodeId, sdoObjId, 2, data, 1));

    if (transmit && parameters.transmission_mode >= PDO_ASYNCHRONOUS_RTR_ONLY)
    {
        uint64_t inhibit_time_us  = parameters.inhibit_time.toMicroseconds();
        if (inhibit_time_us > 65535)
            throw std::invalid_argument("inhibit time too big (must be lower than 6.5s)");
        toLittleEndian(data, static_cast<uint16_t>(inhibit_time_us / 100));
        messages.push_back(
            makeSDOInitiateDomainDownload(nodeId, sdoObjId, 3, data, 2));

        uint64_t timer_period_ms = parameters.timer_period.toMilliseconds();
        if (timer_period_ms > 65535)
            throw std::invalid_argument("timer period too big (must be lower than 65s)");
        toLittleEndian(data, static_cast<uint16_t>(timer_period_ms));
        messages.push_back(
            makeSDOInitiateDomainDownload(nodeId, sdoObjId, 5, data, 2));
    }

    return messages;
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

uint16_t canopen_master::getPDODefaultCOBID(bool transmit, int pdoIndex, uint16_t nodeId)
{
    if (transmit)
        return FUNCTION_PDO0_TRANSMIT + (pdoIndex << 8) + nodeId;
    else
        return FUNCTION_PDO0_RECEIVE  + (pdoIndex << 8) + nodeId;
}

int canopen_master::getPDOIndex(uint16_t functionCode)
{
    return (functionCode - FUNCTION_PDO0_TRANSMIT) >> 8;
}

uint16_t canopen_master::getPDOParametersObjectId(bool transmit, uint8_t pdoIndex)
{
    return (transmit ? 0x1800 : 0x1400) + pdoIndex;
}
uint16_t canopen_master::getPDOMappingObjectId(bool transmit, uint8_t pdoIndex)
{
    return (transmit ? 0x1A00 : 0x1600) + pdoIndex;
}

std::vector<canbus::Message> canopen_master::makePDOMappingMessages(bool transmit, uint8_t nodeId, uint8_t pdoIndex, PDOMapping const& mapping)
{
    std::vector<canbus::Message> result;
    uint16_t pdoObjectId = getPDOMappingObjectId(transmit, pdoIndex);
    uint8_t mappingSize = mapping.mappings.size();

    uint8_t buffer[4] = { 0, 0, 0, 0 };
    result.push_back(makeSDOInitiateDomainDownload(nodeId, pdoObjectId, 0, buffer, 4));
    for (int i = 0; i < mappingSize; ++i)
    {
        PDOMapping::MappedObject m = mapping.mappings[i];

        uint8_t buffer[4];
        buffer[0] = m.size * 8;
        buffer[1] = m.subId;
        toLittleEndian(buffer + 2, m.objectId);
        result.push_back(makeSDOInitiateDomainDownload(nodeId, pdoObjectId, i + 1, buffer, 4));
    }

    toLittleEndian(buffer, static_cast<int32_t>(mappingSize));
    result.push_back(makeSDOInitiateDomainDownload(nodeId, pdoObjectId, 0, buffer, 4));
    return result;
}
