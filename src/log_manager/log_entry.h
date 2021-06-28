#pragma once

#include "common/id.h"

namespace raftcpp {

/**
 * The class to represent a log entry.
 */
struct LogEntry final {
    TermID term_id;

    uint64_t log_index;

    std::string data;

    MSGPACK_DEFINE(term_id, log_index, data);
};

}  // namespace raftcpp
