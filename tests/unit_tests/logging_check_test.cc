#include "common/logging.h"
#include "gtest/gtest.h"

TEST(LogCheckTest, TestRaftcppCheck) {
    RAFTCPP_CHECK(true);
    RAFTCPP_CHECK(false);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
