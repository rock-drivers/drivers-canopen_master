#include <canopen_master/Slave.hpp>
#include <canopen_master/Update.hpp>
#include <canopen_master/Objects.hpp>

using namespace canopen_master;

canbus::Message canopen_master::querySync() {
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x80;
    msg.size = 0;
    return msg;
}

Slave::Slave(uint8_t nodeId)
    : mCANOpen(nodeId) {
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

Update Slave::process(canbus::Message const& message) {
    StateMachine::Update canUpdate = mCANOpen.process(message);
    switch(canUpdate.mode) {
        case StateMachine::PROCESSED_HEARTBEAT:
            return Update::UpdatedObjects(UPDATE_HEARTBEAT);
        case StateMachine::PROCESSED_SDO_INITIATE_DOWNLOAD:
        {
            // Ack of a upload request
            auto object = canUpdate.updated[0];
            return Update::Ack(object.first, object.second);
        }

        default:
            return Update();
    }
}

StateMachine& Slave::getStateMachine() {
    return mCANOpen;
}
