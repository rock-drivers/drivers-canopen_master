#ifndef CANOPEN_MASTER_EMERGENCY_HPP
#define CANOPEN_MASTER_EMERGENCY_HPP

#include <canopen_master/Frame.hpp>
#include <tuple>

namespace canopen_master
{
    struct Emergency
    {
        uint16_t code;
        uint8_t errorRegister;
        uint8_t vendorSpecific[5];
    };

    Emergency parseEmergencyMessage(canbus::Message const& msg);
}

#endif
