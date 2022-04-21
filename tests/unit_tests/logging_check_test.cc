#include "gtest/gtest.h"
#include "src/common/logging.h"

TEST(LogCheckTest, TestRaftcppCheck) {
    RAFTCPP_CHECK(true);
    // TODO(luhuanbing): Better solution？
    // RAFTCPP_CHECK(false);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
