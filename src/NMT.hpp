#ifndef CANOPEN_MASTER_NMT_HPP
#define CANOPEN_MASTER_NMT_HPP

#include <canopen_master/Frame.hpp>
#include <tuple>

namespace canopen_master
{
    canbus::Message makeModuleControlCommand(NODE_STATE_TRANSITION state, uint8_t nodeId);
    std::tuple<uint8_t, NODE_STATE> parseHeartBeat(canbus::Message const& msg);

    canbus::Message makeNMTNodeGuard(uint8_t nodeId);
}

#endif
