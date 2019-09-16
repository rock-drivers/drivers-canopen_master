#include <gtest/gtest.h>
#include <canopen_master/Update.hpp>
#include <canopen_master/Objects.hpp>

using namespace canopen_master;

struct UpdateTest : public ::testing::Test {
};

TEST_F(UpdateTest, it_indicates_if_it_is_not_an_ack) {
    Update update;
    ASSERT_FALSE(update.isAck());
}

TEST_F(UpdateTest, it_indicates_if_it_is_an_ack) {
    Update ack(Update::Ack(0x100, 1));
    ASSERT_TRUE(ack.isAck());
}

TEST_F(UpdateTest, it_allows_to_test_which_objects_it_acked) {
    Update ack(Update::Ack(0x100, 1));
    ASSERT_TRUE(ack.isAcked(0x100, 1));
    ASSERT_FALSE(ack.isAcked(0x101, 1));
    ASSERT_FALSE(ack.isAcked(0x100, 2));
}

CANOPEN_DEFINE_OBJECT(0x100, 1, Test_100_1, uint32_t);
CANOPEN_DEFINE_OBJECT(0x200, 1, Test_200_1, uint32_t);
CANOPEN_DEFINE_OBJECT(0x100, 2, Test_100_2, uint32_t);

TEST_F(UpdateTest, it_allows_to_test_which_objects_it_acked_from_the_object) {
    Update ack(Update::Ack(0x100, 1));
    ASSERT_TRUE(ack.isAcked<Test_100_1>());
    ASSERT_FALSE(ack.isAcked<Test_200_1>());
    ASSERT_FALSE(ack.isAcked<Test_100_2>());
}

TEST_F(UpdateTest, it_allows_to_provide_an_offset_from_the_object_type) {
    Update ack(Update::Ack(0x200, 2));
    ASSERT_FALSE(ack.isAcked<Test_100_1>(0x100, 0));
    ASSERT_FALSE(ack.isAcked<Test_100_1>(0x99, 1));
    ASSERT_TRUE(ack.isAcked<Test_100_1>(0x100, 1));
}

TEST_F(UpdateTest, it_merges_the_update_bitfields) {
    Update update(Update::UpdatedObjects(0x7));
    update.merge(Update::UpdatedObjects(0x8));
    ASSERT_TRUE(update.isUpdated(0xf));
}

TEST_F(UpdateTest, it_tests_whether_all_are_updated) {
    Update update(Update::UpdatedObjects(0x7));
    ASSERT_TRUE(update.isUpdated(0x5));
    ASSERT_FALSE(update.isUpdated(0xc));
}

TEST_F(UpdateTest, it_tests_whether_one_is_updated_among_many) {
    Update update(Update::UpdatedObjects(0x3));
    ASSERT_TRUE(update.hasOneUpdated(0x6));
    ASSERT_FALSE(update.hasOneUpdated(0x4));
}