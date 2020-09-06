#include "logging.h"

namespace raftcpp {
static spdlog::level::level_enum GetMappedSeverity(RaftcppLogLevel severity) {
    switch (severity) {
    case RaftcppLogLevel::RLL_DEBUG:
        return spdlog::level::debug;
    case RaftcppLogLevel::RLL_INFO:
        return spdlog::level::info;
    case RaftcppLogLevel::RLL_WARNING:
        return spdlog::level::warn;
    case RaftcppLogLevel::RLL_ERROR:
        return spdlog::level::err;
    case RaftcppLogLevel::RLL_FATAL:
        return spdlog::level::critical;
    default:
        return spdlog::level::critical;
    }
}

RaftcppLog::RaftcppLog(const char *file_name, int line_number, RaftcppLogLevel severity)
    : filename_(file_name),
      line_number_(line_number),
      log_level_(std::move(severity)),
      is_enabled_(severity >= severity_threshold_) {}

RaftcppLog::~RaftcppLog() {
    try {
        if (is_enabled_) {
            logging_provider->log(GetMappedSeverity(log_level_), "in {} line:{} {}",
                                  filename_, line_number_, ss_.str());
        }
    } catch (const spdlog::spdlog_ex &ex) {
        std::cout << "logging_provider->log failed: " << ex.what() << std::endl;
    }
}

void RaftcppLog::StartRaftcppLog(const std::string &log_file_name,
                                 RaftcppLogLevel severity, uint32_t log_file_roll_size_mb,
                                 uint32_t log_file_roll_cout) {
    severity_threshold_ = severity;
    if (logging_provider == nullptr) {
        try {
            logging_provider = ::spdlog::rotating_logger_mt(
                "raftcpp_log", log_file_name, 1024 * 1024 * 1024 * log_file_roll_size_mb,
                log_file_roll_cout);
            spdlog::set_level(GetMappedSeverity(severity));
            logging_provider->flush_on(spdlog::level::debug);
        } catch (const spdlog::spdlog_ex &ex) {
            std::cout << "RaftcppLog failed: " << ex.what() << std::endl;
        }
    }
}

bool RaftcppLog::IsEnabled() const { return is_enabled_; }

bool RaftcppLog::IsLevelEnabled(RaftcppLogLevel log_level) {
    return log_level >= severity_threshold_;
}

void RaftcppLog::ShutDownRaftcppLog() { spdlog::shutdown(); }

std::shared_ptr<spdlog::logger> RaftcppLog::logging_provider = nullptr;
RaftcppLogLevel RaftcppLog::severity_threshold_ = RaftcppLogLevel::RLL_NOLEVEL;

}  // namespace raftcpp
