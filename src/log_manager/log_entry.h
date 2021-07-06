#pragma once

#include <sstream>

#include "common/id.h"

namespace raftcpp {

/**
 * The class to represent a log entry.
 */
struct LogEntry final {
    TermID term_id;

    uint64_t log_index;

    std::string data;

    std::string toString() {
        return "{\n\ttermId: " + std::to_string(term_id.getTerm()) + "," +
               "\n\tlogIndex: " + std::to_string(log_index) + "," +
               "\n\tdata: " + data.substr(0, 100) + " \n}";
    };

    std::stringstream &operator<<(std::stringstream &ss) {
        ss.str(this->toString());
        return ss;
    }

    MSGPACK_DEFINE(term_id, log_index, data);
};

}  // namespace raftcpp
