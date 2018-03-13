#include <canopen_master/NMT.hpp>
#include <canopen_master/Frame.hpp>

using namespace canopen_master;

canbus::Message canopen_master::makeModuleControlCommand(NODE_STATE_TRANSITION state, uint8_t nodeId)
{
    canbus::Message msg;
    msg.can_id = BROADCAST_NMT_MODULE_CONTROL;
    msg.data[0] = state;
    msg.data[1] = nodeId;
    return msg;
}

canbus::Message canopen_master::makeNMTNodeGuard(uint8_t nodeId)
{
    canbus::Message msg;
    msg.can_id = FUNCTION_NMT_HEARTBEAT + nodeId;
    return msg;
}

std::tuple<uint8_t, NODE_STATE> canopen_master::parseHeartBeat(canbus::Message const& msg)
{
    if (!testFunctionCode(msg, FUNCTION_NMT_HEARTBEAT))
        throw std::invalid_argument("expected a heartbeat message");
    return std::make_tuple(getNodeID(msg), static_cast<NODE_STATE>(msg.data[0]));
}
