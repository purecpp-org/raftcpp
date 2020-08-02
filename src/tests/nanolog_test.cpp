#include <iostream>
#include <string>
#include "gtest/gtest.h"
#include "nanolog.hpp"

class TestNanoLog : public testing::Test {
public:
    static void SetUpTestCase() {
        std::cout << "TestNanoLog SetUpTestCase" << std::endl;
    }

    static void TearDownTestCase() {
        std::cout << "TestNanoLog TearDownTestCase" << std::endl;
    }
};

TEST_F(TestNanoLog, TestLogLevel) {
    std::string log_directory = "/tmp/";
    std::string log_file_name = "test_nanolog";
    nanolog::initialize(nanolog::GuaranteedLogger(), log_directory, log_file_name, 1);
    uint8_t n = 6;
    while (n--) {
        LOG_DEBUG << "Test Line";
        LOG_INFO << "Test Line";
        LOG_WARN << "Test Line";
        LOG_CRIT << "Test Line";
    }

    uint32_t m_file_number = 0;
    std::string real_log_file_path = (log_directory + log_file_name);
    real_log_file_path.append(".");
    real_log_file_path.append(std::to_string(++m_file_number));
    real_log_file_path.append(".txt");

    //reset nanologger to call NanoLogger::pop, so that all the log can be written to file.
    nanolog::initialize(nanolog::GuaranteedLogger(), "", "", 1);

    std::fstream file(real_log_file_path);
    ASSERT_TRUE(file);

    std::string line;
    uint8_t debug_count=0;
    uint8_t info_count = 0;
    uint8_t warn_count = 0;
    uint8_t crit_count = 0;
    while (getline(file, line)) {
        if (line.find("DEBUG") != std::string::npos) {
            debug_count++;
        } else if (line.find("INFO") != std::string::npos) {
            info_count++;
        } else if (line.find("WARN") != std::string::npos) {
            warn_count++;
        } else if (line.find("CRIT") != std::string::npos) {
            crit_count++;
        }
    }

    ASSERT_EQ(debug_count, 6);
    ASSERT_EQ(info_count, 6);
    ASSERT_EQ(warn_count, 6);
    ASSERT_EQ(crit_count, 6);

    std::remove(real_log_file_path.data());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
