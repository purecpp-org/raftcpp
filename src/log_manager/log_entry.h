#pragma once

#include "common/id.h"
#include <iostream>

namespace raftcpp {

/**
 * The class to represent a log entry.
 */
struct LogEntry final {
    TermID term_id;

    uint64_t log_index;

    std::string data;

    std::string toString() {
        return "{termId: " + std::to_string(term_id.getTerm()) + "," +
        "logIndex: " + std::to_string(log_index) + "," +
        "data: " + data.substr(0,100) +" }";
    };
    std::ostream& operator <<(std::ostream &os) {
        return os << toString();
    }

    MSGPACK_DEFINE(term_id, log_index, data);
};

}  // namespace raftcpp
