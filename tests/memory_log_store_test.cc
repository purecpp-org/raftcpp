#include "../src/memory_log_store.h"

#include <iostream>

#include "nanolog.hpp"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("MemoryLogEntry-AppendLogEntry") {
    using namespace raftcpp;

    memory_log_store mem_log_store;
    REQUIRE(mem_log_store.append_entry({}));

    log_entry entry{1, EntryType::APPEND_LOG};
    REQUIRE(mem_log_store.append_entry({}));
    REQUIRE(mem_log_store.append_entry(entry));
    REQUIRE(mem_log_store.append_entry({2}));
    REQUIRE(mem_log_store.entry_at(0).first);

    {
        auto [r, en] = mem_log_store.entry_at(1);
        REQUIRE_EQ(en.term, entry.term);
        REQUIRE(en.type == entry.type);
    }
    {
        auto [r, en] = mem_log_store.entry_at(10);
        REQUIRE_FALSE(r);
    }
}

TEST_CASE("MemoryLogEntry-AppendLogEntries") {
    using namespace raftcpp;
    memory_log_store mem_log_store;

    std::vector<log_entry> entries;
    for (int i = 0; i < 100; i++) {
        entries.push_back({i, EntryType::APPEND_LOG});
    }
    REQUIRE_EQ(mem_log_store.append_entries(entries), 100);
    REQUIRE_EQ(mem_log_store.last_log_index(), 99);
    REQUIRE_EQ(mem_log_store.first_log_index(), 0);
    REQUIRE_EQ(mem_log_store.term_at(1), 1);
    REQUIRE_EQ(mem_log_store.term_at(99), 99);
    REQUIRE_EQ(mem_log_store.term_at(100), 0);
}

TEST_CASE("MemoryLogEntry-TruncatePrefix") {
    using namespace raftcpp;

    memory_log_store mem_log_store;

    std::vector<log_entry> entries;
    for (int i = 0; i < 100; i++) {
        entries.push_back({i, EntryType::APPEND_LOG});
    }
    REQUIRE_EQ(mem_log_store.append_entries(entries), 100);
    REQUIRE(mem_log_store.truncate_prefix(50));
    REQUIRE_EQ(mem_log_store.last_log_index(), 99);
    REQUIRE_EQ(mem_log_store.first_log_index(), 50);
    REQUIRE_EQ(mem_log_store.term_at(1), 0);
    REQUIRE_EQ(mem_log_store.term_at(50), 50);
    REQUIRE_EQ(mem_log_store.term_at(99), 99);
    REQUIRE_EQ(mem_log_store.term_at(100), 0);
}
