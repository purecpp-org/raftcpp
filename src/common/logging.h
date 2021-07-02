#pragma once

#include <iostream>
#include <sstream>
#include <string>

#include "common/id.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/spdlog.h"

namespace raftcpp {
enum class RaftcppLogLevel {
    RLL_DEBUG,
    RLL_INFO,
    RLL_WARNING,
    RLL_ERROR,
    RLL_FATAL,
    RLL_NOLEVEL
};
class RaftcppLogBase {
public:
    virtual ~RaftcppLogBase(){};

    virtual bool IsEnabled() const { return false; };

    template <typename T>
    RaftcppLogBase &operator<<(const T &t) {
        if (IsEnabled()) {
            ss_ << t;
        }
        return *this;
    }

    RaftcppLogBase &operator<<(NodeID &id) {
        if (IsEnabled()) {
            id << ss_;
        }
        return *this;
    }

protected:
    std::stringstream ss_;
};

class RaftcppLog : public RaftcppLogBase {
public:
    RaftcppLog(const char *file_name, int line_number, RaftcppLogLevel severity);

    ~RaftcppLog();

    static void StartRaftcppLog(const std::string &log_file_name,
                                RaftcppLogLevel severity, uint32_t log_file_roll_size_mb,
                                uint32_t log_file_roll_cout);

    bool IsEnabled() const;

    static bool IsLevelEnabled(RaftcppLogLevel log_level);

    static void ShutDownRaftcppLog();

private:
    bool is_enabled_;
    RaftcppLogLevel log_level_;
    std::string filename_;
    int line_number_;
    static std::shared_ptr<spdlog::logger> logging_provider;
    static RaftcppLogLevel severity_threshold_;

protected:
};

class Voidify {
public:
    Voidify() { std::abort(); }

    void operator&(RaftcppLogBase &) {}
};

#define RAFTCPP_LOG_INTERNAL(level) ::raftcpp::RaftcppLog(__FILE__, __LINE__, level)
#define RAFTCPP_LOG(level)                                                    \
    if (raftcpp::RaftcppLog::IsLevelEnabled(raftcpp::RaftcppLogLevel::level)) \
    RAFTCPP_LOG_INTERNAL(raftcpp::RaftcppLogLevel::level)

#define RAFTCPP_LOG_ENABLED(level) \
    raftcpp::RaftcppLog::IsLevelEnabled(raftcpp::RaftcppLogLevel::level)
#define RAFTCPP_IGNORE_EXPR(expr) ((void)(expr))
#define RAFTCPP_CHECK(condition)                                                 \
    (condition) ? RAFTCPP_IGNORE_EXPR(0)                                         \
                : ::raftcpp::Voidify() &                                         \
                      ::raftcpp::RaftcppLog(__FILE__, __LINE__,                  \
                                            raftcpp::RaftcppLogLevel::RLL_FATAL) \
                          << " Check failed: " #condition " "
}  // namespace raftcpp
