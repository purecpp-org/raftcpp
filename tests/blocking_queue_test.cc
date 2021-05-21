#include <chrono>
#include <thread>

#include "log_manager/blocking_queue_mutex_impl.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

using namespace raftcpp;

struct LogEntryTest {
    LogEntryTest() : index_(1), term_(1) {}
    LogEntryTest(int64_t index, int64_t term) : index_(index), term_(term) {}
    LogEntryTest(const LogEntryTest &entry) {
        index_ = entry.index_;
        term_ = entry.term_;
    }

    int64_t index_;
    int64_t term_;
};

bool g_flag = false;
void thread_func(BlockingQueueMutexImpl<LogEntryTest> &log_manager) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    g_flag = true;
    LogEntryTest log_entry{123, 456};
    log_manager.Push(log_entry);
}

TEST_CASE("LogManagerTest") {
    using namespace raftcpp;

    BlockingQueueMutexImpl<LogEntryTest> log_manager;
    LogEntryTest log_entry{123, 456};
    log_manager.Push(log_entry);
    LogEntryTest res = log_manager.Pop();
    REQUIRE_EQ(log_entry.index_, res.index_);
    REQUIRE_EQ(log_entry.term_, res.term_);
}

TEST_CASE("LogManagerEmptyTest, NonBlocking") {
    using namespace raftcpp;

    BlockingQueueMutexImpl<LogEntryTest> log_manager;
    LogEntryTest log_entry;
    bool res = log_manager.Pop(log_entry);
    REQUIRE_EQ(res, false);
}

TEST_CASE("LogManagerTest, Blocking") {
    using namespace raftcpp;

    BlockingQueueMutexImpl<LogEntryTest> log_manager;
    REQUIRE_EQ(g_flag, false);
    std::thread th(thread_func, std::ref(log_manager));
    LogEntryTest res;
    res = log_manager.Pop();
    REQUIRE_EQ(g_flag, true);
    REQUIRE_EQ(res.index_, 123);
    REQUIRE_EQ(res.term_, 456);
    th.join();
}
