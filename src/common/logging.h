#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

namespace raftcpp {

    enum class LogLevel {
        DEBUG,
        INFO,
        WARNING,
        FATAL,
    };

    enum class RaftcppLogLevel {
        RAFTCPP_TRACE,
        RAFTCPP_DEBUG,
        RAFTCPP_INFO,
        RAFTCPP_WARN,
        RAFTCPP_ERROR,
        RAFTCPP_CRITICAL,
        RAFTCPP_NOLEVEL,
    };

    class RaftcppLogBase {
    public:
        virtual ~RaftcppLogBase() {};

        virtual bool IsEnabled() const { return false; };

        template<typename T>
        RaftcppLogBase &operator<<(const T &t) {
            if (IsEnabled()) {
                ss_ << t;
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

        static void
        StartRaftcppLog(const std::string &log_file_name, RaftcppLogLevel severity, uint32_t log_file_roll_size_mb,
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

    class LogOutStream final {
    public:
        explicit LogOutStream(LogLevel log_level) : log_level_(std::move(log_level)) {}

        template<typename T>
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

    class Voidify {
    public:
        Voidify() {}

        void operator&(RaftcppLogBase &) {}
    };

#define RAFTCPP_LOG(LOG_LEVEL) LogOutStream(LOG_LEVEL)
#define RAFTCPP_LOG_INTERNAL(level) ::raftcpp::RaftcppLog(__FILE__, __LINE__, level)
#define RAFTCPP_LOG2(level) \
    if (raftcpp::RaftcppLog::IsLevelEnabled(raftcpp::RaftcppLogLevel::level)) \
    RAFTCPP_LOG_INTERNAL(raftcpp::RaftcppLogLevel::level)

#define RAFTCPP_LOG_ENABLED(level) raftcpp::RaftcppLog::IsLevelEnabled(raftcpp::RaftcppLogLevel::level)
#define RAFTCPP_IGNORE_EXPR(expr) ((void)(expr))
#define RAFTCPP_CHECK(condition)                                                          \
  (condition)                                                                         \
      ? RAFTCPP_IGNORE_EXPR(0)                                                            \
      : ::raftcpp::Voidify() & ::raftcpp::RaftcppLog(__FILE__, __LINE__, raftcpp::RaftcppLogLevel::RAFTCPP_CRITICAL) \
                               << " Check failed: " #condition " "

#define DEBUG LogLevel::DEBUG
#define INFO LogLevel::INFO
#define WARNING LogLevel::WARNING
#define FATAL LogLevel::FATAL

}  // namespace raftcpp

