#include <chrono>
#include <thread>

#include "log_manager/log_manager_mutex_impl.h"

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
void thread_func(LogManagerMutexImpl<LogEntryTest> &log_manager) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    g_flag = true;
    LogEntryTest log_entry{123, 456};
    log_manager.Push(log_entry);
}

TEST_CASE("LogManagerTest") {
    using namespace raftcpp;

    LogManagerMutexImpl<LogEntryTest> log_manager;
    LogEntryTest log_entry{123, 456};
    log_manager.Push(log_entry);
    LogEntryTest res = log_manager.Pop();
    REQUIRE_EQ(log_entry.index_, res.index_);
    REQUIRE_EQ(log_entry.term_, res.term_);
}

TEST_CASE("LogManagerEmptyTest, NonBlocking") {
    using namespace raftcpp;

    LogManagerMutexImpl<LogEntryTest> log_manager;
    LogEntryTest log_entry;
    bool res = log_manager.Pop(log_entry);
    REQUIRE_EQ(res, false);
}

TEST_CASE("LogManagerTest, Blocking") {
    using namespace raftcpp;

    LogManagerMutexImpl<LogEntryTest> log_manager;
    REQUIRE_EQ(g_flag, false);
    std::thread th(thread_func, std::ref(log_manager));
    LogEntryTest res;
    res = log_manager.Pop();
    REQUIRE_EQ(g_flag, true);
    REQUIRE_EQ(res.index_, 123);
    REQUIRE_EQ(res.term_, 456);
    th.join();
}

TEST_CASE("LogManager write entry") {
    ::system("rm -rf  raftcpp_data/");
    LogManagerMutexImpl<raftcpp::LogEntry> log_manager("raftcpp_data");
    log_manager.init();

    // append entry
    for (int i = 0; i < 10; i++) {
        LogEntry entry;
        entry.term_ = 1;
        entry.index_ = i + 1;
        entry.data_ = "hello, raftcpp: " + std::to_string(i + 1);
        REQUIRE_EQ(0, log_manager.append_entry(entry));
    }
    // get entries count
    REQUIRE_EQ(10, log_manager.get_count());

    // read entry
    for (int i = 0; i < 10; i++) {
        int64_t term = log_manager.get_term(i + 1);
        REQUIRE_EQ(term, 1);

        LogEntry entry = log_manager.get_LogEntry(i + 1);
        REQUIRE_EQ(entry.term_, 1);
        REQUIRE_EQ(entry.index_, i + 1);

        char data_buf[128] = "";
        snprintf(data_buf, sizeof(data_buf), "hello, raftcpp: %d", i + 1);
        REQUIRE_EQ(data_buf, entry.data_);
    }

    // batch append entries
    std::vector<LogEntry> logentries;
    for (int i = 10; i < 20; i++) {
        LogEntry entry;
        entry.term_ = 1;
        entry.index_ = i + 1;
        entry.data_ = "hello, raftcpp: " + std::to_string(i + 1);
        logentries.push_back(entry);
    }

    REQUIRE_EQ(0, log_manager.append_entries(logentries));

    // get entries count
    REQUIRE_EQ(20, log_manager.get_count());

    // read entry
    for (int i = 0; i < 20; i++) {
        int64_t term = log_manager.get_term(i + 1);
        REQUIRE_EQ(term, 1);

        LogEntry entry = log_manager.get_LogEntry(i + 1);
        REQUIRE_EQ(entry.term_, 1);
        REQUIRE_EQ(entry.index_, i + 1);

        char data_buf[128] = "";
        snprintf(data_buf, sizeof(data_buf), "hello, raftcpp: %d", i + 1);
        REQUIRE_EQ(data_buf, entry.data_);
    }

    LogEntry entry = log_manager.get_LogEntry(0);
    REQUIRE_EQ(entry.term_, 0);

    LogEntry entry2 = log_manager.get_LogEntry(100);
    REQUIRE_EQ(entry2.term_, 0);

    // truncate entries
    REQUIRE_EQ(0, log_manager.truncate(10));
    REQUIRE_EQ(10, log_manager.get_count());
    for (int i = 0; i < 10; i++) {
        int64_t term = log_manager.get_term(i + 1);
        REQUIRE_EQ(term, 1);

        LogEntry entry = log_manager.get_LogEntry(i + 1);
        REQUIRE_EQ(entry.term_, 1);
        REQUIRE_EQ(entry.index_, i + 1);
    }

    // read
    std::vector<LogEntry> LogEntries;
    for (int i = 10; i < 20; i++) {
        LogEntry entry;
        entry.term_ = 1;
        entry.index_ = i + 1;
        entry.data_ = "HELLO, RAFTCPP: " + std::to_string(i + 1);
        LogEntries.push_back(entry);
    }
    REQUIRE_EQ(0, log_manager.append_entries(LogEntries));

    for (int i = 0; i < 20; i++) {
        int64_t term = log_manager.get_term(i + 1);
        REQUIRE_EQ(term, 1);

        LogEntry entry = log_manager.get_LogEntry(i + 1);
        REQUIRE_EQ(entry.term_, 1);
        REQUIRE_EQ(entry.index_, i + 1);

        char data_buf[128] = "";
        if (i < 10)
            snprintf(data_buf, sizeof(data_buf), "hello, raftcpp: %d", i + 1);
        else
            snprintf(data_buf, sizeof(data_buf), "HELLO, RAFTCPP: %d", i + 1);
        REQUIRE_EQ(data_buf, entry.data_);
    }
}

TEST_CASE("LogManager load entry") {
    LogManagerMutexImpl<raftcpp::LogEntry> log_manager("raftcpp_data");
    log_manager.init();
    // get entries count
    REQUIRE_EQ(20, log_manager.get_count());
    // truncate entries
    REQUIRE_EQ(0, log_manager.truncate(19));
    REQUIRE_EQ(19, log_manager.get_count());

    LogEntry entry;
    entry.term_ = 1;
    entry.index_ = 20;
    entry.data_ = "HELLO, RAFTCPP: 20";
    REQUIRE_EQ(0, log_manager.append_entry(entry));

    entry.term_ = 1;
    entry.index_ = 22;
    entry.data_ = "HELLO, RAFTCPP: 22";
    REQUIRE_EQ(-1, log_manager.append_entry(entry));
}
