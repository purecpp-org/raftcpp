#include <chrono>
#include <thread>

#include "gtest/gtest.h"
#include "log_manager/blocking_queue_mutex_impl.h"

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

TEST(LogManagerTest, TestLogManager) {
    using namespace raftcpp;

    BlockingQueueMutexImpl<LogEntryTest> log_manager;
    LogEntryTest log_entry{123, 456};
    log_manager.Push(log_entry);
    LogEntryTest res = log_manager.Pop();
    ASSERT_EQ(log_entry.index_, res.index_);
    ASSERT_EQ(log_entry.term_, res.term_);
}

TEST(LogManagerTest, TestNonBlocking) {
    using namespace raftcpp;

    BlockingQueueMutexImpl<LogEntryTest> log_manager;
    LogEntryTest log_entry;
    bool res = log_manager.Pop(log_entry);
    ASSERT_EQ(res, false);
}

TEST(LogManagerTest, TestBlocking) {
    using namespace raftcpp;

    BlockingQueueMutexImpl<LogEntryTest> log_manager;
    ASSERT_EQ(g_flag, false);
    std::thread th(thread_func, std::ref(log_manager));
    LogEntryTest res;
    res = log_manager.Pop();
    ASSERT_EQ(g_flag, true);
    ASSERT_EQ(res.index_, 123);
    ASSERT_EQ(res.term_, 456);
    th.join();
}

TEST(BlockingQueueTest, TestMostFront) {
    using namespace raftcpp;

    BlockingQueueMutexImpl<LogEntryTest> log_manager;
    LogEntryTest log_entry0{1, 5};
    LogEntryTest log_entry1{2, 6};
    LogEntryTest log_entry2{3, 7};
    LogEntryTest log_entry3{4, 8};
    log_manager.Push(log_entry0);
    log_manager.Push(log_entry1);
    log_manager.Push(log_entry2);
    log_manager.Push(log_entry3);
    std::vector<LogEntryTest> v = log_manager.MostFront(3);
    ASSERT_EQ(v.size(), 3);
    ASSERT_EQ(v[0].index_, log_entry0.index_);
    ASSERT_EQ(v[1].index_, log_entry1.index_);
    ASSERT_EQ(v[2].index_, log_entry2.index_);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
