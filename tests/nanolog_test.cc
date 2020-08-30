#include "nanolog.hpp"

#include <iostream>
#include <string>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// nanolog INFO (WARN) and doctest INFO (WARN) conflict
#ifdef INFO
#undef INFO
#endif

#ifdef WARN
#undef WARN
#endif

class TestNanoLog {
public:
    static void SetUpTestCase() { std::cout << "TestNanoLog SetUpTestCase" << std::endl; }

    static void TearDownTestCase() {
        std::cout << "TestNanoLog TearDownTestCase" << std::endl;
    }
};

TEST_CASE_FIXTURE(TestNanoLog, "TestLogLevel") {
#ifdef _WIN32
    // define something for Windows (32-bit and 64-bit, this part is common)
    std::string log_directory = "";
#else __linux__
    std::string log_directory = "/tmp/";
#endif

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

    // reset nanologger to call NanoLogger::pop, so that all the log can be written to
    // file.
    nanolog::initialize(nanolog::GuaranteedLogger(), "", "", 1);

    std::fstream file(real_log_file_path);
    REQUIRE(file);

    std::string line;
    uint8_t debug_count = 0;
    uint8_t info_count = 0;
    uint8_t warn_count = 0;
    uint8_t crit_count = 0;
    while (getline(file, line)) {
        if (line.find("DEBUG") != std::string::npos) {
#ifdef _WIN32
            // define something for Windows (32-bit and 64-bit, this part is common)
#ifdef _DEBUG
            debug_count++;
#else
            // debug_count++;
#endif
#else __linux__
#ifndef NDEBUG
            // debug_count++;
#else
            debug_count++;
#endif
#endif
        } else if (line.find("INFO") != std::string::npos) {
            info_count++;
        } else if (line.find("WARN") != std::string::npos) {
            warn_count++;
        } else if (line.find("CRIT") != std::string::npos) {
            crit_count++;
        }
    }

#ifdef _WIN32
    // define something for Windows (32-bit and 64-bit, this part is common)
#ifdef _DEBUG
    REQUIRE_EQ(debug_count, 6);
#else
    REQUIRE_EQ(debug_count, 0);
#endif
#else __linux__
#ifndef NDEBUG
    REQUIRE_EQ(debug_count, 0);
#else
    REQUIRE_EQ(debug_count, 6);
#endif
#endif

    REQUIRE_EQ(info_count, 6);
    REQUIRE_EQ(warn_count, 6);
    REQUIRE_EQ(crit_count, 6);

    std::remove(real_log_file_path.data());
}
