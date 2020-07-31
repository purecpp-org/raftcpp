#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "nanolog.hpp"
#include "memory_log_store.hpp"
#include <iostream>

TEST(MemoryLogEntry, LoggingTest) {
  nanolog::initialize(nanolog::GuaranteedLogger(), "/tmp/", "nanolog", 1);
  LOG_INFO << "Sample NanoLog: " << 1 << 2.5 << 'c';
  // TODO(qwang): assert the logging text is correct.
}

TEST(MemoryLogEntry, AppendLogEntry) {
  using namespace raftcpp;

  memory_log_store mem_log_store;
  ASSERT_TRUE(mem_log_store.append_entry({}));

  log_entry entry{ 1, EntryType::APPEND_LOG };
  ASSERT_TRUE(mem_log_store.append_entry({}));
  ASSERT_TRUE(mem_log_store.append_entry(entry));
  ASSERT_TRUE(mem_log_store.append_entry({2}));
  ASSERT_TRUE(mem_log_store.entry_at(0).first);

  {
    auto [r, en] = mem_log_store.entry_at(1);
    ASSERT_EQ(en.term, entry.term);
    ASSERT_TRUE(en.type == entry.type);
  }
  {
    auto[r, en] = mem_log_store.entry_at(10);
    ASSERT_FALSE(r);
  }
}

TEST(MemoryLogEntry, AppendLogEntries) {
  using namespace raftcpp;
  memory_log_store mem_log_store;

  std::vector<log_entry> entries;
  for (int i = 0; i < 100; i++) {
    entries.push_back({ i, EntryType::APPEND_LOG });
  }
  ASSERT_EQ(mem_log_store.append_entries(entries), 100);
  ASSERT_EQ(mem_log_store.last_log_index(), 99);
  ASSERT_EQ(mem_log_store.first_log_index(), 0);
  ASSERT_EQ(mem_log_store.term_at(1), 1);
  ASSERT_EQ(mem_log_store.term_at(99), 99);
  ASSERT_EQ(mem_log_store.term_at(100), 0);
}

TEST(MemoryLogEntry, TruncatePrefix) {
  using namespace raftcpp;

  memory_log_store mem_log_store;

  std::vector<log_entry> entries;
  for (int i = 0; i < 100; i++) {
    entries.push_back({ i, EntryType::APPEND_LOG });
  }
  ASSERT_EQ(mem_log_store.append_entries(entries), 100);
  ASSERT_TRUE(mem_log_store.truncate_prefix(50));
  ASSERT_EQ(mem_log_store.last_log_index(), 99);
  ASSERT_EQ(mem_log_store.first_log_index(), 50);
  ASSERT_EQ(mem_log_store.term_at(1), 0);
  ASSERT_EQ(mem_log_store.term_at(50), 50);
  ASSERT_EQ(mem_log_store.term_at(99), 99);
  ASSERT_EQ(mem_log_store.term_at(100), 0);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
