#pragma once
#include <string>
#include <vector>
#include <msgpack.hpp>

enum EntryType {
	ENTRY_TYPE_UNKNOWN = 0,
	ENTRY_TYPE_NO_OP = 1,
	ENTRY_TYPE_DATA = 2,
	ENTRY_TYPE_CONFIGURATION = 3
};

enum ErrorType {
	ERROR_TYPE_NONE = 0,
	ERROR_TYPE_LOG = 1,
	ERROR_TYPE_STABLE = 2,
	ERROR_TYPE_SNAPSHOT = 3,
	ERROR_TYPE_STATE_MACHINE = 4
};

namespace raftcpp {
	struct entry_meta{
		int64_t term;
		int32_t type; //EntryType

		MSGPACK_DEFINE(term, type);
	};

	struct snapshot_meta {
		int64_t last_included_index;
		int64_t last_included_term;
		std::string peers;
		std::string old_peers;
		MSGPACK_DEFINE(last_included_index, last_included_term, peers, old_peers);
	};

	struct request_vote_req {
		std::string group_id;
		std::string server_id;
		std::string peer_id;
		int64_t term;
		int64_t last_log_term;
		int64_t last_log_index;
		bool pre_vote;
		MSGPACK_DEFINE(group_id, server_id, peer_id, term, last_log_term, last_log_index, pre_vote);
	};

	struct request_vote_resp {
		int64_t term;
		bool granted;
		MSGPACK_DEFINE(term, granted);
	};

	struct append_entries_req {
		std::string group_id;
		std::string server_id;
		std::string peer_id;
		int64_t term;
		int64_t prev_log_term;
		int64_t prev_log_index;
		std::vector<entry_meta> entries;
		int64_t committed_index;
		std::string data;//optional

		MSGPACK_DEFINE(group_id, server_id, peer_id, term, prev_log_term, prev_log_index, entries, committed_index, data);
	};

	struct append_entries_resp {
		int64_t term;
		bool success;
		int64_t last_log_index;
		MSGPACK_DEFINE(term, success, last_log_index);
	};
}