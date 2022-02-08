#include "common/file.h"

#include "gtest/gtest.h"

TEST(FileTest, TestWriteToFile) {
    using namespace raftcpp;

    File test_file = File::Open("./tmp.dat");
    std::string str_test1 = "hello, world";
    std::string str_test2 = "hello";
    test_file.CleanAndWrite(str_test1);
    std::string str_res1 = test_file.ReadAll();
    ASSERT_EQ(str_res1, str_test1);
    test_file.CleanAndWrite(str_test2);
    std::string str_res2 = test_file.ReadAll();
    ASSERT_EQ(str_res2, str_test2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
