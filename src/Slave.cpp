#include <canopen_master/Slave.hpp>
#include <canopen_master/Objects.hpp>

using namespace canopen_master;

canbus::Message canopen_master::querySync() {
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x80;
    msg.size = 0;
    return msg;
}

Slave::Slave(StateMachine& state_machine)
    : mCANOpen(state_machine) {
}

Slave::~Slave() {

}

canbus::Message Slave::queryNodeState() const {
    return mCANOpen.queryState();
}

canbus::Message Slave::queryNodeStateTransition(
    NODE_STATE_TRANSITION transition
) const {
    return mCANOpen.queryStateTransition(transition);
}

NODE_STATE Slave::getNodeState() const {
    return mCANOpen.getState();
}

#define CANOPEN_MODE_UPDATE_CASE(mode, update_id) \
    case canopen_master::StateMachine::mode: \
        update |= update_id; \
        break;

StateMachine::Update Slave::process(canbus::Message const& message) {
    return mCANOpen.process(message);
}

canbus::Message Slave::getRPDOMessage(unsigned int pdoIndex) const {
    return mCANOpen.getRPDOMessage(pdoIndex);
}
