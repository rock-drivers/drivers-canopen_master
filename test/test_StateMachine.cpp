#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <canopen_master/StateMachine.hpp>
#include <canopen_master/SDO.hpp>

using namespace std;
using namespace canopen_master;
using ::testing::ElementsAre;

TEST(StateMachine, hasNoStateBeforeTheFirstMessage)
{
    StateMachine machine(2);
    ASSERT_EQ(false, machine.hasState());
}

TEST(StateMachine, bootupMessageHandling) {
    StateMachine machine(2);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x702;
    msg.data[0] = 0;
    ASSERT_EQ(true, machine.process(msg));
    ASSERT_EQ(true, machine.hasState());
    ASSERT_EQ(msg.time, machine.getLastMessageTime());
    ASSERT_EQ(msg.time, machine.getLastStateUpdate());
    ASSERT_EQ(NODE_INITIALIZING, machine.getState());
}

TEST(StateMachine, stateUpdateMessage) {
    StateMachine machine(2);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x702;
    msg.data[0] = NODE_STOPPED;
    ASSERT_EQ(true, machine.process(msg));
    ASSERT_EQ(true, machine.hasState());
    ASSERT_EQ(msg.time, machine.getLastMessageTime());
    ASSERT_EQ(msg.time, machine.getLastStateUpdate());
    ASSERT_EQ(NODE_STOPPED, machine.getState());
}

TEST(StateMachine, stateUpdateFromAnotherNode) {
    StateMachine machine(2);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x703;
    msg.data[0] = NODE_STOPPED;
    ASSERT_EQ(false, machine.process(msg));
    ASSERT_EQ(false, machine.hasState());
}

TEST(StateMachine, processThrowsOnAnEmergencyMessage) {
    StateMachine machine(2);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x082;
    msg.data[0] = 0x10;
    msg.data[1] = 0x10;
    ASSERT_THROW(machine.process(msg), EmergencyMessageReceived);
}

TEST(StateMachine, processDoesNotThrowOnAnEmergencyWithZeroCode) {
    StateMachine machine(2);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x082;
    msg.data[0] = 0x10;
    msg.data[1] = 0x00;
    ASSERT_EQ(true, machine.process(msg));
}

TEST(StateMachine, processIgnoredEmergencyMessagesFromOtherNodes) {
    StateMachine machine(2);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x083;
    msg.data[0] = 0x00;
    msg.data[1] = 0x01;
    ASSERT_EQ(false, machine.process(msg));
}

TEST(StateMachine, getNodeID) {
    ASSERT_EQ(StateMachine(10).getNodeID(), 10);
}

TEST(StateMachine, upload) {
    StateMachine machine(2);
    canbus::Message msg = machine.upload(0x1801, 3);
    ASSERT_EQ(msg.can_id, 0x602);
    EXPECT_THAT(
        std::vector<uint8_t>(msg.data, msg.data + 8),
        ElementsAre(0x40, 0x01, 0x18, 0x03, 0x00, 0x00, 0x00, 0x00)
    );
}

TEST(StateMachine, download) {
    StateMachine machine(2);
    uint16_t value = 0x3FE;
    canbus::Message msg = machine.download(0x1801, 3, value);
    ASSERT_EQ(msg.can_id, 0x602);
    EXPECT_THAT(
        std::vector<uint8_t>(msg.data, msg.data + 8),
        ElementsAre(0x2B, 0x01, 0x18, 0x03, 0xFE, 0x03, 0x00, 0x00)
    );
}

TEST(StateMachine, getFailsOnADeclaredButUnreadObject) {
    StateMachine machine(2);
    machine.declare(0x1801, 3, 2);
    ASSERT_THROW(machine.get<uint16_t>(0x1801, 3), ObjectNotRead);
}

TEST(StateMachine, downloadOfADeclaredObject) {
    StateMachine machine(2);
    uint16_t value = 0x3FE;
    machine.declare(0x1801, 3, 2);
    canbus::Message msg = machine.download(0x1801, 3, value);
    ASSERT_EQ(msg.can_id, 0x602);
    EXPECT_THAT(
        std::vector<uint8_t>(msg.data, msg.data + 8),
        ElementsAre(0x2B, 0x01, 0x18, 0x03, 0xFE, 0x03, 0x00, 0x00)
    );
}

TEST(StateMachine, downloadFailsIfDeclaredSizeMismatches) {
    StateMachine machine(2);
    machine.declare(0x1801, 3, 4);
    ASSERT_THROW(machine.download(0x1801, 3, static_cast<uint16_t>(0)), ObjectSizeMismatch);
}

TEST(StateMachine, processUploadReply) {
    StateMachine machine(2);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x582;
    msg.data[0] = 0x4B;
    msg.data[1] = 0x01;
    msg.data[2] = 0x18;
    msg.data[3] = 0x03;
    msg.data[4] = 0xFE;
    msg.data[5] = 0x03;
    msg.data[6] = 0x00;
    msg.data[7] = 0x00;
    ASSERT_EQ(true, machine.process(msg));
    ASSERT_EQ(0x3FE, machine.get<uint16_t>(0x1801, 3));
}

TEST(StateMachine, processSDOAbort)
{
    canbus::Message msg;
    msg.can_id = 0x582;
    msg.data[0] = static_cast<uint8_t>(SDO_ABORT_DOMAIN_TRANSFER << 5);
    msg.data[1] = 0xFE;
    msg.data[2] = 0x03;
    msg.data[3] = 0x10;
    msg.data[4] = 0x05;
    msg.data[5] = 0x00;
    msg.data[6] = 0x03;
    msg.data[7] = 0x05;

    StateMachine machine(2);
    ASSERT_THROW(machine.process(msg), SDODomainTransferAborted);
}

TEST(StateMachine, ignoresSDOAbortForAnotherNode)
{
    canbus::Message msg;
    msg.can_id = 0x583;
    msg.data[0] = SDO_ABORT_DOMAIN_TRANSFER;
    msg.data[1] = 0xFE;
    msg.data[2] = 0x03;
    msg.data[3] = 0x10;
    msg.data[4] = 0x05;
    msg.data[5] = 0x00;
    msg.data[6] = 0x03;
    msg.data[7] = 0x05;
    StateMachine machine(2);
    ASSERT_EQ(false, machine.process(msg));
}

TEST(StateMachine, configurePDOMapping)
{
    PDOMapping mappings;
    mappings.add(0x6000, 0x02, 1);
    mappings.add(0x6401, 0x01, 2);
    StateMachine machine(2);
    vector<canbus::Message> msg = machine.configurePDOMapping(true, 1, mappings);
    ASSERT_EQ(3, msg.size());

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[0].data + 1));
    ASSERT_EQ(0,          msg[0].data[3]);
    ASSERT_EQ(2,          msg[0].data[4]);

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[1].data + 1));
    ASSERT_EQ(1,          msg[1].data[3]);
    ASSERT_EQ(0x60000208, fromLittleEndian<uint32_t>(msg[1].data + 4));

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[2].data + 1));
    ASSERT_EQ(2,          msg[2].data[3]);
    ASSERT_EQ(0x64010110, fromLittleEndian<uint32_t>(msg[2].data + 4));
}

TEST(StateMachine, configurePDOMappingValidatesTheSizesMatchesDeclaredMappings)
{
    PDOMapping mappings;
    mappings.add(0x6000, 0x02, 1);
    StateMachine machine(2);
    machine.declare(0x6000, 0x02, 2);
    ASSERT_THROW(machine.configurePDOMapping(true, 1, mappings),
        ObjectSizeMismatch);
}

TEST(StateMachine, processPDO)
{
    PDOMapping mappings;
    mappings.add(0x6000, 0x02, 1);
    mappings.add(0x6401, 0x01, 2);
    StateMachine machine(2);
    machine.declarePDOMapping(1, mappings);

    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = FUNCTION_PDO1_TRANSMIT + 2;
    msg.data[0] = 0x01;
    msg.data[1] = 0x02;
    msg.data[2] = 0x03;
    ASSERT_EQ(true, machine.process(msg));
    ASSERT_EQ(1, machine.sizeOf(0x6000, 0x02));
    ASSERT_EQ(2, machine.sizeOf(0x6401, 0x01));
    ASSERT_EQ(0x01, machine.get<uint8_t>(0x6000, 0x02));
    ASSERT_EQ(0x0302, machine.get<uint16_t>(0x6401, 0x01));
}

TEST(StateMachine, processPDOReturnsFalseIfNoMappingExists)
{
    StateMachine machine(2);

    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = FUNCTION_PDO1_TRANSMIT + 2;
    ASSERT_EQ(false, machine.process(msg));
}

TEST(StateMachine, processPDOReturnsFalseIfAMappingIsEmpty)
{
    StateMachine machine(2);
    PDOMapping mappings;
    machine.declarePDOMapping(1, mappings);

    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = FUNCTION_PDO1_TRANSMIT + 2;
    ASSERT_EQ(false, machine.process(msg));
}
