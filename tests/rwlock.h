#pragma once

#include <climits>
#include <condition_variable>
#include <mutex>

class ReaderWriterLock {
    using mutex_t = std::mutex;
    using cond_t = std::condition_variable;
    static const uint32_t MAX_READERS = UINT_MAX;

public:
    ReaderWriterLock() = default;
    ~ReaderWriterLock() { std::lock_guard<mutex_t> guard(mutex_); }

    ReaderWriterLock(const ReaderWriterLock &latch) = delete;
    ReaderWriterLock &operator=(const ReaderWriterLock &latch) = delete;

    void w_lock() {
        std::unique_lock<mutex_t> latch(mutex_);
        while (writer_entered_) {
            reader_.wait(latch);
        }
        writer_entered_ = true;
        while (reader_count_ > 0) {
            writer_.wait(latch);
        }
    }

    void w_unlock() {
        std::lock_guard<mutex_t> guard(mutex_);
        writer_entered_ = false;
        reader_.notify_all();
    }

    void r_lock() {
        std::unique_lock<mutex_t> latch(mutex_);
        while (writer_entered_ || reader_count_ == MAX_READERS) {
            reader_.wait(latch);
        }
        reader_count_++;
    }

    void r_unlock() {
        std::lock_guard<mutex_t> guard(mutex_);
        reader_count_--;
        if (writer_entered_) {
            if (reader_count_ == 0) {
                writer_.notify_one();
            }
        } else {
            if (reader_count_ == MAX_READERS - 1) {
                reader_.notify_one();
            }
        }
    }

private:
    mutex_t mutex_;
    cond_t writer_;
    cond_t reader_;
    uint32_t reader_count_{0};
    bool writer_entered_{false};
};