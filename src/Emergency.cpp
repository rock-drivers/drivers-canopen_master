#include <canopen_master/Emergency.hpp>
#include <canopen_master/Frame.hpp>
#include <cstring>

using namespace canopen_master;

Emergency canopen_master::parseEmergencyMessage(canbus::Message const& msg)
{
    if (getFunctionCode(msg) != FUNCTION_EMERGENCY)
        throw std::invalid_argument("expected an emergency message");
    Emergency em;
    em.code = fromLittleEndian<uint16_t>(msg.data);
    std::memcpy(em.vendorSpecific, msg.data + 3, 5);
    return em;
}
