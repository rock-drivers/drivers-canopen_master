#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include <canopen_master/StateMachine.hpp>
#include <canopen_master/SDO.hpp>

using namespace std;
using namespace canopen_master;
typedef StateMachine::Update Update;
using ::testing::ElementsAre;

TEST(StateMachine, hasNoStateBeforeTheFirstMessage)
{
    StateMachine machine(2);
    ASSERT_EQ(false, machine.hasState());
}

TEST(StateMachine, queryStateTransition) {
    StateMachine machine(2);
    canbus::Message msg = machine.queryStateTransition(NODE_START);
    ASSERT_EQ(msg.can_id, 0);
    ASSERT_EQ(msg.size, 2);
    ASSERT_EQ(msg.data[0], 1);
    ASSERT_EQ(msg.data[1], 2);
}

TEST(StateMachine, queryState) {
    StateMachine machine(2);
    canbus::Message msg = machine.queryState();
    ASSERT_EQ(msg.can_id, 0x702);
    ASSERT_EQ(msg.size, 0);
}

TEST(StateMachine, bootupMessageHandling) {
    StateMachine machine(2);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x702;
    msg.data[0] = 0;
    ASSERT_EQ(Update(StateMachine::PROCESSED_HEARTBEAT), machine.process(msg));
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
    ASSERT_EQ(Update(StateMachine::PROCESSED_HEARTBEAT), machine.process(msg));
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
    ASSERT_EQ(Update(StateMachine::PROCESSED_NOT_FOR_ME), machine.process(msg));
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
    ASSERT_EQ(Update(StateMachine::PROCESSED_EMERGENCY_NO_ERROR), machine.process(msg));
}

TEST(StateMachine, processIgnoredEmergencyMessagesFromOtherNodes) {
    StateMachine machine(2);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x083;
    msg.data[0] = 0x00;
    msg.data[1] = 0x01;
    ASSERT_EQ(Update(StateMachine::PROCESSED_NOT_FOR_ME), machine.process(msg));
}

TEST(StateMachine, getNodeID) {
    ASSERT_EQ(StateMachine(10).getNodeID(), 10);
}

TEST(StateMachine, upload) {
    StateMachine machine(2);
    canbus::Message msg = machine.upload(0x1801, 3);
    ASSERT_EQ(msg.can_id, 0x602);
    ASSERT_EQ(msg.size, 8);
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
    ASSERT_EQ(msg.size, 8);
    EXPECT_THAT(
        std::vector<uint8_t>(msg.data, msg.data + 8),
        ElementsAre(0x2B, 0x01, 0x18, 0x03, 0xFE, 0x03, 0x00, 0x00)
    );
}

TEST(StateMachine, download_8) {
    StateMachine machine(2);
    canbus::Message msg = machine.download<uint8_t>(0x1801, 3, 0xFE);
    ASSERT_EQ(msg.can_id, 0x602);
    ASSERT_EQ(msg.size, 8);
    EXPECT_THAT(
        std::vector<uint8_t>(msg.data, msg.data + 8),
        ElementsAre(0x2F, 0x01, 0x18, 0x03, 0xFE, 0x00, 0x00, 0x00)
    );
}

TEST(StateMachine, download_16) {
    StateMachine machine(2);
    canbus::Message msg = machine.download<uint16_t>(0x1801, 3, 0x1234);
    ASSERT_EQ(msg.can_id, 0x602);
    ASSERT_EQ(msg.size, 8);
    EXPECT_THAT(
        std::vector<uint8_t>(msg.data, msg.data + 8),
        ElementsAre(0x2B, 0x01, 0x18, 0x03, 0x34, 0x12, 0x00, 0x00)
    );
}

TEST(StateMachine, download_32) {
    StateMachine machine(2);
    canbus::Message msg = machine.download<uint32_t>(0x1801, 3, 0x12345678);
    ASSERT_EQ(msg.can_id, 0x602);
    ASSERT_EQ(msg.size, 8);
    EXPECT_THAT(
        std::vector<uint8_t>(msg.data, msg.data + 8),
        ElementsAre(0x23, 0x01, 0x18, 0x03, 0x78, 0x56, 0x34, 0x12)
    );
}


TEST(StateMachine, download_unknown) {
    StateMachine machine(2, true);
    std::vector<uint8_t> raw { 0xFE, 0x00, 0x00, 0x00 };
    canbus::Message msg = machine.download(0x1801, 3, raw.data(), raw.size());
    ASSERT_EQ(msg.can_id, 0x602);
    ASSERT_EQ(msg.size, 8);
    EXPECT_THAT(
        std::vector<uint8_t>(msg.data, msg.data + 8),
        ElementsAre(0x22, 0x01, 0x18, 0x03, 0xFE, 0x00, 0x00, 0x00)
    );
}

TEST(StateMachine, set_and_get) {
    StateMachine machine(2);
    uint16_t value = 0x1234;
    base::Time time = base::Time::fromSeconds(12);
    machine.set(0x12, 0x1, value, time);
    ASSERT_EQ(value, machine.get<uint16_t>(0x12, 0x1));
    ASSERT_EQ(time, machine.timestamp(0x12, 0x1));
}

TEST(StateMachine, setRejectsZeroUpdateTime) {
    StateMachine machine(2);
    uint16_t value = 0x1234;
    ASSERT_THROW(machine.set(0x12, 0x1, value, base::Time()), std::invalid_argument);
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
    ASSERT_EQ(msg.size, 8);
    EXPECT_THAT(
        std::vector<uint8_t>(msg.data, msg.data + 8),
        ElementsAre(0x2B, 0x01, 0x18, 0x03, 0xFE, 0x03, 0x00, 0x00)
    );
}

TEST(StateMachine, downloadFailsIfDeclaredSizeMismatches) {
    StateMachine machine(2);
    machine.declare(0x1801, 3, 4);
    ASSERT_THROW(
        machine.download(0x1801, 3, static_cast<uint16_t>(0)),
        ObjectSizeMismatch);
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
    ASSERT_EQ(Update(StateMachine::PROCESSED_SDO, 0x1801, 3), machine.process(msg));
    ASSERT_EQ(0x3FE, machine.get<uint16_t>(0x1801, 3));
}

TEST(StateMachine, processUnknownSizeUploadReply) {
    StateMachine machine(2, true);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x582;
    msg.data[0] = 0x42;
    msg.data[1] = 0x01;
    msg.data[2] = 0x18;
    msg.data[3] = 0x03;
    msg.data[4] = 0xFE;
    msg.data[5] = 0x03;
    msg.data[6] = 0xFF;
    msg.data[7] = 0xFF;
    ASSERT_EQ(Update(StateMachine::PROCESSED_SDO, 0x1801, 3), machine.process(msg));
    ASSERT_EQ(0x3FE, machine.get<uint16_t>(0x1801, 3));
}

TEST(StateMachine, processUnknownSizeUploadReplyOfAnAlreadyDefinedObject) {
    StateMachine machine(2, true);
    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = 0x582;
    msg.data[0] = 0x42;
    msg.data[1] = 0x01;
    msg.data[2] = 0x18;
    msg.data[3] = 0x03;
    msg.data[4] = 0xFE;
    msg.data[5] = 0x03;
    msg.data[6] = 0xFF;
    msg.data[7] = 0xFF;
    machine.process(msg);

    msg.data[4] = 0xEF;
    msg.data[5] = 0xBE;
    machine.process(msg);
    ASSERT_EQ(0xBEEF, machine.get<uint16_t>(0x1801, 3));
}

TEST(StateMachine, processUploadReplyRejectsZeroUpdateTime) {
    StateMachine machine(2);
    canbus::Message msg;
    msg.time = base::Time();
    msg.can_id = 0x582;
    msg.data[0] = 0x4B;
    msg.data[1] = 0x01;
    msg.data[2] = 0x18;
    msg.data[3] = 0x03;
    msg.data[4] = 0xFE;
    msg.data[5] = 0x03;
    msg.data[6] = 0x00;
    msg.data[7] = 0x00;
    ASSERT_THROW(machine.process(msg), ProtocolError);
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
    ASSERT_EQ(Update(StateMachine::PROCESSED_NOT_FOR_ME), machine.process(msg));
}

TEST(StateMachine, configurePDOMapping)
{
    PDOMapping mappings;
    mappings.add(0x6000, 0x02, 1);
    mappings.add(0x6401, 0x01, 2);
    StateMachine machine(2);
    vector<canbus::Message> msg = machine.configurePDOMapping(true, 1, mappings);
    ASSERT_EQ(4, msg.size());

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[0].data + 1));
    ASSERT_EQ(0,          msg[0].data[3]);
    ASSERT_EQ(0,          fromLittleEndian<uint32_t>(msg[0].data + 4));

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[1].data + 1));
    ASSERT_EQ(1,          msg[1].data[3]);
    ASSERT_EQ(0x60000208, fromLittleEndian<uint32_t>(msg[1].data + 4));

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[2].data + 1));
    ASSERT_EQ(2,          msg[2].data[3]);
    ASSERT_EQ(0x64010110, fromLittleEndian<uint32_t>(msg[2].data + 4));

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[3].data + 1));
    ASSERT_EQ(0,          msg[0].data[3]);
    ASSERT_EQ(2,          fromLittleEndian<uint32_t>(msg[3].data + 4));
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
    machine.declareTPDOMapping(1, mappings);

    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = FUNCTION_PDO1_TRANSMIT + 2;
    msg.data[0] = 0x01;
    msg.data[1] = 0x02;
    msg.data[2] = 0x03;

    Update expected(StateMachine::PROCESSED_PDO);
    expected.addUpdate(0x6000, 0x02);
    expected.addUpdate(0x6401, 0x01);
    ASSERT_EQ(expected, machine.process(msg));
    ASSERT_EQ(1, machine.sizeOf(0x6000, 0x02));
    ASSERT_EQ(2, machine.sizeOf(0x6401, 0x01));
    ASSERT_EQ(0x01, machine.get<uint8_t>(0x6000, 0x02));
    ASSERT_EQ(0x0302, machine.get<uint16_t>(0x6401, 0x01));
}

TEST(StateMachine, processPDOIfNoMappingExists)
{
    StateMachine machine(2);

    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = FUNCTION_PDO1_TRANSMIT + 2;
    ASSERT_EQ(Update(StateMachine::PROCESSED_PDO_UNEXPECTED), machine.process(msg));
}

TEST(StateMachine, processPDOIfAMappingIsEmpty)
{
    StateMachine machine(2);
    PDOMapping mappings;
    machine.declareTPDOMapping(1, mappings);

    canbus::Message msg;
    msg.time = base::Time::now();
    msg.can_id = FUNCTION_PDO1_TRANSMIT + 2;
    ASSERT_EQ(Update(StateMachine::PROCESSED_PDO_UNEXPECTED), machine.process(msg));
}

TEST(StateMachine, configurePDOParameters_receive_asynchronous)
{
    StateMachine machine(2);
    PDOCommunicationParameters parameters;
    parameters.transmission_mode = PDO_ASYNCHRONOUS;

    vector<canbus::Message> messages =
        machine.configurePDOParameters(false, 1, parameters);

    ASSERT_EQ(2, messages.size());
    ASSERT_EQ(0x1401, fromLittleEndian<uint16_t>(messages[0].data + 1));
    ASSERT_EQ(1,      fromLittleEndian<uint8_t>(messages[0].data + 3));
    ASSERT_EQ(0x302,  fromLittleEndian<uint32_t>(messages[0].data + 4));

    ASSERT_EQ(0x1401, fromLittleEndian<uint16_t>(messages[1].data + 1));
    ASSERT_EQ(2,      fromLittleEndian<uint8_t>(messages[1].data + 3));
    ASSERT_EQ(254,    fromLittleEndian<uint8_t>(messages[1].data + 4));
}

TEST(StateMachine, configurePDOParameters_transmit_synchronous)
{
    StateMachine machine(2);
    PDOCommunicationParameters parameters;
    parameters.transmission_mode = PDO_SYNCHRONOUS;
    parameters.sync_period = 10;

    vector<canbus::Message> messages =
        machine.configurePDOParameters(true, 1, parameters);

    ASSERT_EQ(2, messages.size());
    ASSERT_EQ(0x1801, fromLittleEndian<uint16_t>(messages[0].data + 1));
    ASSERT_EQ(1,      fromLittleEndian<uint8_t>(messages[0].data + 3));
    ASSERT_EQ(0x282,  fromLittleEndian<uint32_t>(messages[0].data + 4));

    ASSERT_EQ(0x1801, fromLittleEndian<uint16_t>(messages[1].data + 1));
    ASSERT_EQ(2,      fromLittleEndian<uint8_t>(messages[1].data + 3));
    ASSERT_EQ(10,     fromLittleEndian<uint8_t>(messages[1].data + 4));
}

TEST(StateMachine, configurePDOParameters_transmit_cyclic_synchronous_rejects_invalid_cycle)
{
    StateMachine machine(2);
    PDOCommunicationParameters parameters;
    parameters.transmission_mode = PDO_SYNCHRONOUS;
    parameters.sync_period = 253;

    ASSERT_THROW(machine.configurePDOParameters(true, 1, parameters),
        std::invalid_argument);
}

TEST(StateMachine, configurePDOParameters_transmit_asynchronous_aperiodic)
{
    StateMachine machine(2);
    PDOCommunicationParameters parameters;
    parameters.transmission_mode = PDO_ASYNCHRONOUS;
    parameters.inhibit_time = base::Time::fromMilliseconds(10);

    vector<canbus::Message> messages =
        machine.configurePDOParameters(true, 1, parameters);

    ASSERT_EQ(4, messages.size());
    ASSERT_EQ(0x1801, fromLittleEndian<uint16_t>(messages[0].data + 1));
    ASSERT_EQ(1,      fromLittleEndian<uint8_t>(messages[0].data + 3));
    ASSERT_EQ(0x282,  fromLittleEndian<uint32_t>(messages[0].data + 4));

    ASSERT_EQ(0x1801, fromLittleEndian<uint16_t>(messages[1].data + 1));
    ASSERT_EQ(2,      fromLittleEndian<uint8_t>(messages[1].data + 3));
    ASSERT_EQ(254,    fromLittleEndian<uint8_t>(messages[1].data + 4));

    ASSERT_EQ(0x1801, fromLittleEndian<uint16_t>(messages[2].data + 1));
    ASSERT_EQ(3,      fromLittleEndian<uint8_t>(messages[2].data + 3));
    ASSERT_EQ(100,    fromLittleEndian<uint16_t>(messages[2].data + 4));

    ASSERT_EQ(0x1801, fromLittleEndian<uint16_t>(messages[3].data + 1));
    ASSERT_EQ(5,      fromLittleEndian<uint8_t>(messages[3].data + 3));
    ASSERT_EQ(0,      fromLittleEndian<uint16_t>(messages[3].data + 4));
}

TEST(StateMachine, configurePDOParameters_transmit_asynchronous_periodic)
{
    StateMachine machine(2);
    PDOCommunicationParameters parameters;
    parameters.transmission_mode = PDO_ASYNCHRONOUS;
    parameters.inhibit_time = base::Time::fromMilliseconds(10);
    parameters.timer_period = base::Time::fromMilliseconds(10);

    vector<canbus::Message> messages =
        machine.configurePDOParameters(true, 1, parameters);

    ASSERT_EQ(4, messages.size());
    ASSERT_EQ(0x1801, fromLittleEndian<uint16_t>(messages[0].data + 1));
    ASSERT_EQ(1,      fromLittleEndian<uint8_t>(messages[0].data + 3));
    ASSERT_EQ(0x282,  fromLittleEndian<uint32_t>(messages[0].data + 4));

    ASSERT_EQ(0x1801, fromLittleEndian<uint16_t>(messages[1].data + 1));
    ASSERT_EQ(2,      fromLittleEndian<uint8_t>(messages[1].data + 3));
    ASSERT_EQ(254,    fromLittleEndian<uint8_t>(messages[1].data + 4));

    ASSERT_EQ(0x1801, fromLittleEndian<uint16_t>(messages[2].data + 1));
    ASSERT_EQ(3,      fromLittleEndian<uint8_t>(messages[2].data + 3));
    ASSERT_EQ(100,    fromLittleEndian<uint16_t>(messages[2].data + 4));

    ASSERT_EQ(0x1801, fromLittleEndian<uint16_t>(messages[3].data + 1));
    ASSERT_EQ(5,      fromLittleEndian<uint8_t>(messages[3].data + 3));
    ASSERT_EQ(10,     fromLittleEndian<uint16_t>(messages[3].data + 4));
}

TEST(StateMachine, configurePDOParameters_transmit_asynchronous_validates_inhibit_time)
{
    StateMachine machine(2);
    PDOCommunicationParameters parameters;
    parameters.transmission_mode = PDO_ASYNCHRONOUS;
    parameters.inhibit_time = base::Time::fromMicroseconds(6553600ul);

    ASSERT_THROW(machine.configurePDOParameters(true, 1, parameters),
        std::invalid_argument);
}

TEST(StateMachine, configurePDOParameters_transmit_asynchronous_validates_period)
{
    StateMachine machine(2);
    PDOCommunicationParameters parameters;
    parameters.transmission_mode = PDO_ASYNCHRONOUS;
    parameters.timer_period = base::Time::fromMilliseconds(65536);

    ASSERT_THROW(machine.configurePDOParameters(true, 1, parameters),
        std::invalid_argument);
}

TEST(StateMachine, configurePDO)
{
    PDOCommunicationParameters parameters;
    parameters.transmission_mode = PDO_ASYNCHRONOUS;
    parameters.inhibit_time = base::Time::fromMilliseconds(10);
    parameters.timer_period = base::Time::fromMilliseconds(10);

    PDOMapping mappings;
    mappings.add(0x6000, 0x02, 1);
    mappings.add(0x6401, 0x01, 2);

    StateMachine machine(2);
    vector<canbus::Message> msg =
        machine.configurePDO(true, 1, parameters, mappings);
    ASSERT_EQ(9, msg.size());

    ASSERT_EQ(0x1801,     fromLittleEndian<uint16_t>(msg[0].data + 1));
    ASSERT_EQ(1,          fromLittleEndian<uint8_t>(msg[0].data + 3));
    ASSERT_EQ(0x80000282, fromLittleEndian<uint32_t>(msg[0].data + 4));

    ASSERT_EQ(0x1801,     fromLittleEndian<uint16_t>(msg[1].data + 1));
    ASSERT_EQ(2,          fromLittleEndian<uint8_t>(msg[1].data + 3));
    ASSERT_EQ(254,        fromLittleEndian<uint8_t>(msg[1].data + 4));

    ASSERT_EQ(0x1801,     fromLittleEndian<uint16_t>(msg[2].data + 1));
    ASSERT_EQ(3,          fromLittleEndian<uint8_t>(msg[2].data + 3));
    ASSERT_EQ(100,        fromLittleEndian<uint16_t>(msg[2].data + 4));

    ASSERT_EQ(0x1801,     fromLittleEndian<uint16_t>(msg[3].data + 1));
    ASSERT_EQ(5,          fromLittleEndian<uint8_t>(msg[3].data + 3));
    ASSERT_EQ(10,         fromLittleEndian<uint16_t>(msg[3].data + 4));

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[4].data + 1));
    ASSERT_EQ(0,          msg[4].data[3]);
    ASSERT_EQ(0,          fromLittleEndian<uint32_t>(msg[4].data + 4));

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[5].data + 1));
    ASSERT_EQ(1,          msg[5].data[3]);
    ASSERT_EQ(0x60000208, fromLittleEndian<uint32_t>(msg[5].data + 4));

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[6].data + 1));
    ASSERT_EQ(2,          msg[6].data[3]);
    ASSERT_EQ(0x64010110, fromLittleEndian<uint32_t>(msg[6].data + 4));

    ASSERT_EQ(0x1A01,     fromLittleEndian<uint16_t>(msg[7].data + 1));
    ASSERT_EQ(0,          msg[7].data[3]);
    ASSERT_EQ(2,          fromLittleEndian<uint32_t>(msg[7].data + 4));

    ASSERT_EQ(0x1801,     fromLittleEndian<uint16_t>(msg[8].data + 1));
    ASSERT_EQ(1,          fromLittleEndian<uint8_t>(msg[8].data + 3));
    ASSERT_EQ(0x282,      fromLittleEndian<uint32_t>(msg[8].data + 4));
}

TEST(StateMachine, configurePDO_COBID_MESSAGE_RESERVED_BIT_QUIRK)
{
    PDOCommunicationParameters parameters;
    parameters.transmission_mode = PDO_ASYNCHRONOUS;

    PDOMapping mappings;
    mappings.add(0x6000, 0x02, 1);
    mappings.add(0x6401, 0x01, 2);

    StateMachine machine(2);
    machine.setQuirks(StateMachine::PDO_COBID_MESSAGE_RESERVED_BIT_QUIRK);
    vector<canbus::Message> msg =
        machine.configurePDO(true, 1, parameters, mappings);

    ASSERT_EQ(9, msg.size());

    ASSERT_EQ(0x1801,     fromLittleEndian<uint16_t>(msg[0].data + 1));
    ASSERT_EQ(1,          fromLittleEndian<uint8_t>(msg[0].data + 3));
    ASSERT_EQ(0xC0000282, fromLittleEndian<uint32_t>(msg[0].data + 4));

    ASSERT_EQ(0x1801,     fromLittleEndian<uint16_t>(msg[8].data + 1));
    ASSERT_EQ(1,          fromLittleEndian<uint8_t>(msg[8].data + 3));
    ASSERT_EQ(0x40000282, fromLittleEndian<uint32_t>(msg[8].data + 4));
}

TEST(StateMachine, disablePDO)
{
    StateMachine machine(2);
    canbus::Message msg = machine.disablePDO(true, 1);

    ASSERT_EQ(0x1801,     fromLittleEndian<uint16_t>(msg.data + 1));
    ASSERT_EQ(1,          fromLittleEndian<uint8_t>(msg.data + 3));
    ASSERT_EQ(0x80000282, fromLittleEndian<uint32_t>(msg.data + 4));
}

TEST(StateMachine, disablePDO_PDO_COBID_MESSAGE_RESERVED_BIT_QUIRK)
{
    StateMachine machine(2);
    machine.setQuirks(StateMachine::PDO_COBID_MESSAGE_RESERVED_BIT_QUIRK);
    canbus::Message msg = machine.disablePDO(true, 1);

    ASSERT_EQ(0x1801,     fromLittleEndian<uint16_t>(msg.data + 1));
    ASSERT_EQ(1,          fromLittleEndian<uint8_t>(msg.data + 3));
    ASSERT_EQ(0xC0000282, fromLittleEndian<uint32_t>(msg.data + 4));
}
TEST(Update, it_is_reporting_IGNORED_MESSAGE_by_default) {
    Update update;
    ASSERT_EQ(StateMachine::PROCESSED_IGNORED_MESSAGE, update.mode);
}

TEST(Update, it_has_no_updated_objects_by_default) {
    Update update;
    ASSERT_FALSE(update.hasUpdatedObjects());
}

TEST(Update, it_reports_that_it_has_updated_objects_once_some_are_added) {
    Update update;
    update.addUpdate(10, 20);
    ASSERT_TRUE(update.hasUpdatedObjects());
}

TEST(Update, it_reports_if_an_object_has_been_updated) {
    Update update;
    update.addUpdate(10, 20);
    ASSERT_TRUE(update.hasUpdatedObject(10, 20));
}

TEST(Update, it_handles_multiple_objects) {
    Update update;
    update.addUpdate(10, 20);
    update.addUpdate(11, 21);
    update.addUpdate(12, 22);
    ASSERT_TRUE(update.hasUpdatedObject(10, 20));
    ASSERT_TRUE(update.hasUpdatedObject(11, 21));
    ASSERT_TRUE(update.hasUpdatedObject(12, 22));
}

struct DictionaryObject {
    static const int OBJECT_ID = 10;
    static const int OBJECT_SUB_ID = 20;
};

TEST(Update, it_adds_an_update_with_a_dictionary_type) {
    Update update;
    update.addUpdate<DictionaryObject>();
    ASSERT_TRUE(update.hasUpdatedObject(10, 20));
}

TEST(Update, it_accepts_an_id_offset_when_adding_an_update_with_a_dictionary_type) {
    Update update;
    update.addUpdate<DictionaryObject>(5, 0);
    ASSERT_TRUE(update.hasUpdatedObject(15, 20));
}

TEST(Update, it_accepts_a_sub_id_offset_when_adding_an_update_with_a_dictionary_type) {
    Update update;
    update.addUpdate<DictionaryObject>(0, 5);
    ASSERT_TRUE(update.hasUpdatedObject(10, 25));
}

TEST(Update, it_can_be_queried_with_a_dictionary_type) {
    Update update;
    update.addUpdate(10, 20);
    ASSERT_TRUE(update.hasUpdatedObject<DictionaryObject>());
}

TEST(Update, it_accepts_an_object_id_offset_when_queries_with_a_dictionary_type) {
    Update update;
    update.addUpdate(10, 25);
    ASSERT_TRUE(update.hasUpdatedObject<DictionaryObject>(0, 5));
}

TEST(Update, it_accepts_an_object_sub_id_offset_when_queries_with_a_dictionary_type) {
    Update update;
    update.addUpdate(15, 20);
    ASSERT_TRUE(update.hasUpdatedObject<DictionaryObject>(5, 0));
}
