
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

// nanolog INFO (WARN) and doctest INFO (WARN) conflict
#ifdef INFO
#undef INFO
#endif

#ifdef WARN
#undef WARN
#endif

#include "common/logging.h"

TEST_CASE("logging_test") {
    using namespace raftcpp;
//    RAFTCPP_LOG(INFO) << "Hello " << 123;
    raftcpp::RaftcppLog::StartRaftcppLog("log/test.log",raftcpp::RaftcppLogLevel::RAFTCPP_INFO,10,3);
    RAFTCPP_LOG2(RAFTCPP_DEBUG) << "this debug message won't show up " << 456;
    RAFTCPP_LOG2(RAFTCPP_WARN) << "Hello " << 123;
    RAFTCPP_LOG2(RAFTCPP_INFO) << "world " << 456 << " 789";
    RAFTCPP_CHECK(true) << "This is a RAFTCPP_CHECK"
                                 << " message but it won't show up";
    raftcpp::RaftcppLog::ShutDownRaftcppLog();
}
