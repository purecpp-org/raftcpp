#pragma once
#include <atomic>
#include <map>
#include <vector>
#include <shared_mutex>
#include "entity.h"

namespace raftcpp {
    class memory_log_store {
    public:
        memory_log_store() = default;

        int64_t first_log_index() const {
            return start_idx_;
        }

        int64_t last_log_index() const {
            std::shared_lock lock(mtx_);
            return start_idx_ + (logs_.empty() ? 0 : logs_.size() - 1);
        }

        int64_t last_log_term() const {
            std::shared_lock lock(mtx_);
            if (logs_.empty())
                return 0;

            auto last = logs_.rbegin();
            auto term = last->second.term;
            return term;
        }

        std::pair<bool, log_entry> entry_at(int64_t index) const {
            std::shared_lock lock(mtx_);
            if (auto it = logs_.find(index); it != logs_.end()) {
                return { true, it->second };
            }

            return {};
        }

        int64_t term_at(int64_t index) const {
            std::shared_lock lock(mtx_);
            if (auto it = logs_.find(index); it != logs_.end()) {
                return it->second.term;
            }

            return 0;
        }

        bool append_entry(log_entry entry) {
            std::unique_lock lock(mtx_);
            size_t idx = start_idx_ + logs_.empty() ? 0 : logs_.size();
            auto it = logs_.emplace(idx, entry);
            return it.second;
        }

        ///**
        // * Append entries to log, return append success number.
        // */
        size_t append_entries(const std::vector<log_entry>& entries) {
            size_t size = entries.size();

            std::unique_lock lock(mtx_);
            size_t idx = start_idx_ + logs_.size();            
            for (size_t i = 0; i < size; i++) {
                if (auto it = logs_.emplace(idx + i, entries[i]); !it.second)
                    return i;
            }

            return size;
        }

        ///**
        // * Delete logs from storage's head, [first_log_index, first_index_kept) will
        // * be discarded.
        // */
        bool truncate_prefix(int64_t first_index_kept) {
            start_idx_ = first_index_kept;
            
            std::unique_lock lock(mtx_);
            for (auto i = 0; i < first_index_kept; i++) {
                if (auto it = logs_.find(i); it != logs_.end()) {
                    logs_.erase(it);
                }
            }
            
            return true;
        }

        ///**
        // * Delete uncommitted logs from storage's tail, (last_index_kept, last_log_index]
        // * will be discarded.
        // */
        bool truncate_suffix(int64_t last_index_kept) {
            //TODO
            return false;
        }

        ///**
        // * Drop all the existing logs and reset next log index to |next_log_index|.
        // * This function is called after installing snapshot from leader.
        // */
        bool reset(int64_t next_log_index) {
            if (next_log_index <= 0)
                return false;

            start_idx_ = next_log_index;
            std::unique_lock lock(mtx_);
            logs_.clear();
        }

    private:
        memory_log_store(const memory_log_store&) = delete;
        memory_log_store& operator=(const memory_log_store&) = delete;
        memory_log_store(memory_log_store&&) = delete;
        memory_log_store& operator=(memory_log_store&&) = delete;

        std::atomic<int64_t> start_idx_ = {0};
        std::map<int64_t, log_entry> logs_;
        mutable std::shared_mutex mtx_;
    };
}
