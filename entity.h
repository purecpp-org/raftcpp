#pragma once
#include <string>
#include <vector>
#include <msgpack.hpp>

namespace raftcpp {
    enum class EntryType {
        APPEND_LOG = 1,
        CONF = 2,
        CLUSTER_SERVER = 3,
        LOG_PACK = 4,
        SNAPSHOT_SYNC_REQ = 5,
        CUSTOM = 999,
    };

    enum class State {
        LEADER,
        CANDIDATE,
        FOLLOWER,
    };

    struct log_entry{
        int64_t term;
        EntryType type;

        MSGPACK_DEFINE(term, (int32_t&)type);
    };

    struct snapshot_meta {
        int64_t last_included_index;
        int64_t last_included_term;
        std::string peers;
        std::string old_peers;

        MSGPACK_DEFINE(last_included_index, last_included_term, peers, old_peers);
    };

    struct vote_req {
        int32_t src;
        int32_t dst;
        int64_t term;
        int64_t last_log_term;
        int64_t last_log_index;
        int32_t port;

        MSGPACK_DEFINE(src, dst, term, last_log_term, last_log_index, port);
    };

    struct vote_resp {
        int64_t term;
        bool granted;

        MSGPACK_DEFINE(term, granted);
    };

    struct append_entries_req {
        int32_t src;
        int32_t dst;
        int64_t term;
        int64_t prev_log_term;
        int64_t prev_log_index;        
        int64_t committed_index;
        std::vector<log_entry> entries;
        //optional
        std::string data;

        MSGPACK_DEFINE(src, dst, term, prev_log_term, prev_log_index, committed_index, entries, data);
    };

    struct append_entries_resp {
        int64_t term;
        bool success;
        int64_t last_log_index;

        MSGPACK_DEFINE(term, success, last_log_index);
    };
}
