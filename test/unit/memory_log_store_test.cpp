#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../doctest/doctest.hpp"

#include "../../memory_log_store.hpp"

//#include "doctest.h"

TEST_CASE("test append log entry") {
    using namespace raftcpp;

    memory_log_store mem_log_store;
    CHECK(mem_log_store.append_entry({}));
    CHECK(mem_log_store.append_entry({}));

    log_entry entry{ 1, EntryType::APPEND_LOG };
    CHECK(mem_log_store.append_entry(entry));
    CHECK(mem_log_store.append_entry({2}));

    CHECK(mem_log_store.entry_at(0).first);

    {
        auto [r, en] = mem_log_store.entry_at(2);
        CHECK(en.term==entry.term);
        CHECK(en.type == entry.type);
    }

    {
        auto [r, en] = mem_log_store.entry_at(3);
        CHECK(en.term == 2);
        CHECK(mem_log_store.last_log_term() == 2);
    }

    {
        auto[r, en] = mem_log_store.entry_at(10);
        CHECK(!r);
    }

}

TEST_CASE("test append log entries") {
    using namespace raftcpp;

    memory_log_store mem_log_store;

    std::vector<log_entry> entries;
    for (int i = 0; i < 100; i++) {
        entries.push_back({ i, EntryType::APPEND_LOG });
    }
    CHECK(mem_log_store.append_entries(entries)==100);
    CHECK(mem_log_store.last_log_index() == 99);
    CHECK(mem_log_store.first_log_index() == 0);
    CHECK(mem_log_store.term_at(1) == 1);
    CHECK(mem_log_store.term_at(99) == 99);
    CHECK(mem_log_store.term_at(100) == 0);
}

TEST_CASE("test truncate_prefix") {
    using namespace raftcpp;

    memory_log_store mem_log_store;

    std::vector<log_entry> entries;
    for (int i = 0; i < 100; i++) {
        entries.push_back({ i, EntryType::APPEND_LOG });
    }
    CHECK(mem_log_store.append_entries(entries) == 100);
    CHECK(mem_log_store.truncate_prefix(50));
    CHECK(mem_log_store.last_log_index() == 99);
    CHECK(mem_log_store.first_log_index() == 50);
    CHECK(mem_log_store.term_at(1) == 0);
    CHECK(mem_log_store.term_at(50) == 50);
    CHECK(mem_log_store.term_at(99) == 99);
    CHECK(mem_log_store.term_at(100) == 0);
}

TEST_CASE("test reset") {
    using namespace raftcpp;

    memory_log_store mem_log_store;

    std::vector<log_entry> entries;
    for (int i = 0; i < 100; i++) {
        entries.push_back({ i, EntryType::APPEND_LOG });
    }
    CHECK(mem_log_store.append_entries(entries)==100);
    CHECK(mem_log_store.last_log_index() == 99);
    CHECK(mem_log_store.first_log_index() == 0);
    CHECK(mem_log_store.term_at(1) == 1);
    CHECK(mem_log_store.term_at(99) == 99);
    CHECK(mem_log_store.term_at(100) == 0);

    mem_log_store.reset(100);
    CHECK(mem_log_store.first_log_index() == 100);
    CHECK(mem_log_store.last_log_index() == 100);
}
