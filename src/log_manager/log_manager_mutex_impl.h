#pragma once

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>

#include "log_manager/log_manager.h"

namespace raftcpp {
#define ENTRY_HEAD_SIZE 12

struct EntryHeader {
    int32_t term;
    uint32_t data_len;
    uint32_t data_checksum;
};

struct LogEntry {
public:
    int64_t index_ = 0;
    int32_t term_ = 0;
    std::string data_ = "";
};

class LogEntryPackage {
public:
    LogEntryPackage() { ; }
    virtual ~LogEntryPackage() { ; }

    // Format of Header(96bits)
    // term(32bits)|data_len(32bits)  |data_checksum(32bits)
    int64_t packHeader(const LogEntry &log, char *headBuf) {
        uint32_t data_len = 0;
        uint32_t sum = 0;
        int32_t len = 0;

        memcpy(headBuf, &log.term_, 4);
        len += 4;

        data_len = log.data_.length();
        memcpy(headBuf + len, &data_len, 4);
        len += 4;

        for (size_t i = 0; i < log.data_.length(); i++) {
            sum = sum ^ log.data_[i];
        }
        memcpy(headBuf + len, &sum, 4);
        len += 4;
        return len;
    }

    void unpackHeader(const char *buf, EntryHeader &header) {
        memcpy(&header.term, buf, 4);
        memcpy(&header.data_len, buf + 4, 4);
        memcpy(&header.data_checksum, buf + 8, 4);
    }

private:
};

template <typename LogEntryType>
class LogManagerMutexImpl : public LogManagerInterface<LogEntryType> {
public:
    LogManagerMutexImpl() = default;

    ~LogManagerMutexImpl() { fd_.close(); }
    LogManagerMutexImpl(const std::string &path) : path_(path), is_open_(false) {}

    virtual LogEntryType Pop() override;

    virtual bool Pop(LogEntryType &log_entry) override;

    virtual void Push(const LogEntryType &log_entry) override;

    int init() {
        if (!std::filesystem::exists(path_)) {
            std::filesystem::create_directory(path_);
        }
        if ((unsigned char)(path_[path_.length() - 1]) == (unsigned char)'/')
            path_ += "log_data";
        else
            path_ += "/log_data";
        std::string path(path_);
        fd_.open(path, std::fstream::in | std::fstream::out | std::fstream::app |
                           std::fstream::binary);
        if (fd_.is_open()) {
            is_open_ = true;
        } else
            return -1;
        int64_t file_size = fd_.tellg();
        if (file_size > 0) {
            fd_.seekg(0, std::ios::beg);  // move to file header
            int64_t entry_off = 0;
            while (!fd_.eof()) {
                char headBuf[ENTRY_HEAD_SIZE];
                memset(headBuf, 0, ENTRY_HEAD_SIZE);
                fd_.read(headBuf, ENTRY_HEAD_SIZE);
                std::streampos start = fd_.tellg();
                if (start == -1) break;

                // unpack
                EntryHeader header;
                LogEntryPackage pack;
                pack.unpackHeader(headBuf, header);
                // verify checksum
                char *dataBuf = new char[header.data_len];
                fd_.read(dataBuf, header.data_len);
                uint32_t sum = 0;
                for (size_t i = 0; i < header.data_len; i++) {
                    sum = sum ^ dataBuf[i];
                }
                if (sum != header.data_checksum) {
                    // TODO: abort()?
                    ;
                }
                offset_and_term_.push_back(std::make_pair(entry_off, header.term));
                entry_off += ENTRY_HEAD_SIZE + header.data_len;
                delete[] dataBuf;
            }
        }
        return 0;
    }

    int append_entry(const LogEntry &entry, bool will_sync = true) {
        std::unique_lock<std::mutex> lock(file_mutex_);
        if (entry.index_ == 0)
            return 0;
        else if (entry.index_ != offset_and_term_.size() + 1) {
            std::cout << "entry.index_=" << entry.index_
                      << " last_index=" << offset_and_term_.size() << std::endl;
            return -1;
        }

        char headBuf[ENTRY_HEAD_SIZE];
        memset(headBuf, 0, ENTRY_HEAD_SIZE);
        LogEntryPackage pack;
        pack.packHeader(entry, headBuf);
        std::string data;
        data.resize(ENTRY_HEAD_SIZE + entry.data_.length());
        memcpy(data.data(), headBuf, ENTRY_HEAD_SIZE);
        memcpy(data.data() + ENTRY_HEAD_SIZE, entry.data_.data(), entry.data_.length());
        fd_.seekg(0, std::ios::end);  // move to file end
        offset_and_term_.push_back(std::make_pair(fd_.tellg(), entry.term_));
        fd_.write(data.c_str(), data.length());
        if (will_sync) fd_.sync();
        return 0;
    }

    int append_entries(const std::vector<LogEntry> &entries, bool will_sync = true) {
        std::unique_lock<std::mutex> lock(file_mutex_);
        if (entries.size() == 0)
            return 0;
        else if (entries.front().index_ != offset_and_term_.size() + 1) {
            std::cout << "entry.index_=" << entries.front().index_
                      << " last_index=" << offset_and_term_.size() << std::endl;
            return -1;
        }
        std::string input_data;
        fd_.seekg(0, std::ios::end);  // move to file end
        int64_t file_size = fd_.tellg();
        for (auto entry : entries) {
            char headBuf[ENTRY_HEAD_SIZE];
            memset(headBuf, 0, ENTRY_HEAD_SIZE);
            LogEntryPackage pack;
            pack.packHeader(entry, headBuf);
            std::string data;
            data.resize(ENTRY_HEAD_SIZE + entry.data_.length());
            memcpy(data.data(), headBuf, ENTRY_HEAD_SIZE);
            memcpy(data.data() + ENTRY_HEAD_SIZE, entry.data_.data(),
                   entry.data_.length());
            input_data.append(data);
            offset_and_term_.push_back(std::make_pair(file_size, entry.term_));
            file_size += ENTRY_HEAD_SIZE + entry.data_.length();
        }
        fd_.write(input_data.c_str(), input_data.length());
        if (will_sync) fd_.sync();
        return 0;
    }

    // get entry by index
    LogEntry get_LogEntry(const int64_t index) {
        std::unique_lock<std::mutex> lock(file_mutex_);
        LogEntry entry;
        if (index > offset_and_term_.size() || index <= 0) return entry;
        fd_.seekg(offset_and_term_[index - 1].first);
        char headBuf[ENTRY_HEAD_SIZE];
        memset(headBuf, 0, ENTRY_HEAD_SIZE);
        fd_.read(headBuf, ENTRY_HEAD_SIZE);
        EntryHeader header;
        LogEntryPackage pack;
        pack.unpackHeader(headBuf, header);
        char *dataBuf = new char[header.data_len];
        memset(dataBuf, 0, header.data_len);
        fd_.read(dataBuf, header.data_len);
        uint32_t sum = 0;
        for (size_t i = 0; i < header.data_len; i++) {
            sum = sum ^ dataBuf[i];
        }
        if (sum != header.data_checksum) {
            // TODO: abort()?
            std::cout << "check sun invalid." << std::endl;
        }

        entry.term_ = header.term;
        entry.index_ = index;
        entry.data_ = dataBuf;
        entry.data_.resize(header.data_len);
        delete[] dataBuf;

        return entry;
    }

    // get entry's term by index
    int64_t get_term(const int64_t index) const {
        if (index > offset_and_term_.size() || index < 0) return -1;
        return offset_and_term_[index - 1].second;
    }
    int64_t get_size(const int64_t index) const {
        if (index > offset_and_term_.size() || index < 0) return -1;
        return offset_and_term_[index - 1].first;
    }
    int64_t get_count() { return offset_and_term_.size(); }
    // truncate data to last_index_kept
    int truncate(const int64_t last_index_kept) {
        std::unique_lock<std::mutex> lock(file_mutex_);
        if (last_index_kept >= offset_and_term_.size()) {
            return 0;
        }
        if (last_index_kept < 0) return -1;
        std::filesystem::resize_file(path_, offset_and_term_[last_index_kept].first);
        //        fd_.seekg(0, std::ios::end);
        if (last_index_kept == offset_and_term_.size() - 1)
            offset_and_term_.pop_back();
        else {
            int64_t diff = offset_and_term_.size() - last_index_kept;
            offset_and_term_.erase(offset_and_term_.end() - diff, offset_and_term_.end());
        }
        return 0;
    }

    bool is_open() const { return is_open_; }

private:
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::queue<LogEntryType> queue_;

    std::mutex file_mutex_;
    std::string path_;
    bool is_open_;
    std::fstream fd_;
    std::vector<std::pair<int64_t /*offset*/, int32_t /*term*/>> offset_and_term_;
};

template <typename LogEntryType>
LogEntryType LogManagerMutexImpl<LogEntryType>::Pop() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_cv_.wait(lock, [this] { return !queue_.empty(); });
    LogEntryType log_entry_type = queue_.front();
    queue_.pop();
    return log_entry_type;
}

template <typename LogEntryType>
bool LogManagerMutexImpl<LogEntryType>::Pop(LogEntryType &log_entry) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    if (!queue_.empty()) {
        log_entry = queue_.front();
        queue_.pop();
        return true;
    }
    return false;
}

template <typename LogEntryType>
void LogManagerMutexImpl<LogEntryType>::Push(const LogEntryType &log_entry) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    queue_.push(log_entry);
    queue_cv_.notify_all();
}

}  // namespace raftcpp
