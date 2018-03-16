#include <canopen_master/Exceptions.hpp>
#include <sstream>

using namespace std;
using namespace canopen_master;

std::string SDODomainTransferAborted::formatMessage(
    uint16_t objectId, uint8_t subId, uint32_t rawCode)
{
    std::ostringstream formatter;
    formatter << "SDO Domain Transfer aborted for object "
        << "0x" << hex << objectId << "/" << dec << static_cast<int>(subId)
        << ", error code: " << hex << rawCode;
    return formatter.str();
}