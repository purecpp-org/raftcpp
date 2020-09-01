#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace raftcpp {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    FATAL,
};

#define DEBUG LogLevel::DEBUG
#define INFO LogLevel::INFO
#define WARNING LogLevel::WARNING
#define FATAL LogLevel::FATAL

class LogOutStream final {
public:
    explicit LogOutStream(LogLevel log_level) : log_level_(std::move(log_level)) {}

    template <typename T>
    LogOutStream &operator<<(const T &t) {
        if (IsEnabled()) {
            ss_ << t;
        }
        return *this;
    }

    bool IsEnabled() const { return true; }

    ~LogOutStream() { std::cout << ss_.str() << std::endl; }

private:
    LogLevel log_level_;
    std::stringstream ss_;
};

#define RAFTCPP_LOG(LOG_LEVEL) LogOutStream(LOG_LEVEL)

}  // namespace raftcpp
