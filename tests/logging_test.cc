
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "common/logging.h"

#include <doctest.h>

TEST_CASE("TestLogLevel") {
    raftcpp::RaftcppLog::StartRaftcppLog("log/test.log", raftcpp::RaftcppLogLevel::RLL_INFO,
                                         10, 3);
    RAFTCPP_LOG(RLL_DEBUG) << "this debug message won't show up " << 456;
    RAFTCPP_LOG(RLL_WARNING) << "Hello " << 123;
    RAFTCPP_LOG(RLL_INFO) << "world " << 456 << " 789";
    RAFTCPP_CHECK(true) << "This is a RAFTCPP_CHECK"
                        << " message but it won't show up";
    raftcpp::RaftcppLog::ShutDownRaftcppLog();
    std::fstream file("log/test.log");
    REQUIRE(file);

    std::string line;
    uint8_t debug_count = 0;
    uint8_t info_count = 0;
    uint8_t warn_count = 0;
    while (getline(file, line)) {
        std::cout << "line:" << line << std::endl;
        if (line.find("debug") != std::string::npos) {
            debug_count++;
        } else if (line.find("info") != std::string::npos) {
            info_count++;
        } else if (line.find("warning") != std::string::npos) {
            warn_count++;
        }
    }

    REQUIRE_EQ(debug_count, 0);
    REQUIRE_EQ(info_count, 1);
    REQUIRE_EQ(warn_count, 1);
    file.close();
    std::remove("log/test.log");
}
