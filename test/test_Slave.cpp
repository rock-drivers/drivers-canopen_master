#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <canopen_master/Slave.hpp>
#include <canopen_master/SDO.hpp>

using namespace base;
using namespace canopen_master;
using testing::ElementsAreArray;

struct SlaveTest : public ::testing::Test {
    StateMachine state_machine;
    Slave slave;

    SlaveTest()
        : state_machine(42)
        , slave(state_machine) {
    }
};

CANOPEN_DEFINE_OBJECT(0x100, 1, Test_100_1, uint32_t);
CANOPEN_DEFINE_OBJECT(0x200, 1, Test_200_1, uint32_t);
CANOPEN_DEFINE_OBJECT(0x100, 2, Test_100_2, uint32_t);

TEST_F(SlaveTest, it_returns_the_query_message_matching_the_given_object) {
    auto query = slave.queryUpload<Test_100_1>();

    ASSERT_EQ(0x100, getSDOObjectID(query));
    ASSERT_EQ(1, getSDOObjectSubID(query));
    ASSERT_EQ(SDO_INITIATE_DOMAIN_UPLOAD, getSDOCommand(query).command);
}

TEST_F(SlaveTest, it_allows_adding_offsets_to_id_and_subid_in_query) {
    auto query = slave.queryUpload<Test_100_1>(1, 2);

    ASSERT_EQ(0x101, getSDOObjectID(query));
    ASSERT_EQ(3, getSDOObjectSubID(query));
    ASSERT_EQ(SDO_INITIATE_DOMAIN_UPLOAD, getSDOCommand(query).command);
}

TEST_F(SlaveTest, it_returns_the_download_message_matching_the_given_object) {
    canbus::Message query = slave.queryDownload<Test_100_1>(0x12345678);

    ASSERT_EQ(0x100, getSDOObjectID(query));
    ASSERT_EQ(1, getSDOObjectSubID(query));
    ASSERT_EQ(SDO_INITIATE_DOMAIN_DOWNLOAD, getSDOCommand(query).command);

    uint8_t expected[] = { 0x78, 0x56, 0x34, 0x12 };
    ASSERT_THAT(std::vector<uint8_t>(query.data + 4, query.data + 8),
                ElementsAreArray(expected));
}

TEST_F(SlaveTest, it_allows_adding_offsets_to_id_and_subid_in_download_query) {
    canbus::Message query = slave.queryDownload<Test_100_1>(0x12345678, 1, 2);

    ASSERT_EQ(0x101, getSDOObjectID(query));
    ASSERT_EQ(3, getSDOObjectSubID(query));
    ASSERT_EQ(SDO_INITIATE_DOMAIN_DOWNLOAD, getSDOCommand(query).command);
}

TEST_F(SlaveTest, it_tests_whether_an_object_is_present) {
    ASSERT_FALSE(slave.has<Test_100_1>());
    slave.set<Test_100_1>(0x12345678);
    ASSERT_TRUE(slave.has<Test_100_1>());
}

TEST_F(SlaveTest, it_allows_adding_offsets_when_testing_presence) {
    ASSERT_FALSE(slave.has<Test_100_1>(1, 3));
    slave.set<Test_100_1>(0x12345678, 1, 3);
    ASSERT_TRUE(slave.has<Test_100_1>(1, 3));
}

TEST_F(SlaveTest, it_returns_the_timestamp_of_the_last_set_value) {
    ASSERT_TRUE(slave.timestamp<Test_100_1>().isNull());
    Time time(Time::fromSeconds(100));
    slave.set<Test_100_1>(0x12345678, time);
    ASSERT_EQ(time, slave.timestamp<Test_100_1>());
}

TEST_F(SlaveTest, it_allows_adding_offsets_when_returning_timestamp) {
    ASSERT_TRUE(slave.timestamp<Test_100_1>(1, 3).isNull());
    Time time(Time::fromSeconds(100));
    slave.set<Test_100_1>(0x12345678, 1, 3, time);
    ASSERT_EQ(time, slave.timestamp<Test_100_1>(1, 3));
}

TEST_F(SlaveTest, it_sets_and_gets_an_object) {
    slave.set<Test_100_1>(0x12345678);
    ASSERT_EQ(0x12345678, slave.get<Test_100_1>());
}

TEST_F(SlaveTest, it_allows_adding_offset_to_id_and_subid_in_get_and_set) {
    slave.set<Test_100_1>(0x12345678, 1, 2);
    ASSERT_EQ(0x12345678, state_machine.get<uint32_t>(0x101, 3));
    auto value = slave.get<Test_100_1>(1, 2);
    ASSERT_EQ(0x12345678, value);
}