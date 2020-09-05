#include "log_manager/log_manager_mutex_impl.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

struct LogEntryTest {
    LogEntryTest() : index_(0), term_(0) {}
    LogEntryTest(int64_t index, int64_t term) : index_(index), term_(term) {}
    LogEntryTest(const LogEntryTest &entry) {
        index_ = entry.index_;
        term_ = entry.term_;
    }

    int64_t index_;
    int64_t term_;
};

TEST_CASE("LogManagerTest") {
    using namespace raftcpp;

    LogManagerMutexImpl<LogEntryTest> log_manager;
    LogEntryTest log_entry{123, 456};
    log_manager.Push(log_entry);
    LogEntryTest res = log_manager.Pop();
    REQUIRE_EQ(log_entry.index_, res.index_);
    REQUIRE_EQ(log_entry.term_, res.term_);
}
