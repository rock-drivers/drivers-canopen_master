#ifndef CANOPEN_MASTER_NMT_HPP
#define CANOPEN_MASTER_NMT_HPP

#include <canopen_master/Frame.hpp>
#include <tuple>

namespace canopen_master
{
    /** Network management command codes */
    enum MODULE_CONTROL_COMMANDS
    {
        GO_TO_OPERATIONAL,
        GO_TO_STOPPED,
        GO_TO_PRE_OPERATIONAL,
        GO_TO_RESET_NODE,
        GO_TO_RESET_COMMUNICATION
    };

    canbus::Message makeModuleControlCommand(MODULE_CONTROL_COMMANDS state, uint8_t nodeId);
    std::tuple<uint8_t, NODE_STATE> parseHeartBeat(canbus::Message const& msg);

    canbus::Message makeNMTNodeGuard(uint8_t nodeId);
}

#endif
