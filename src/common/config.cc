#include "common/config.h"

#include <set>

namespace raftcpp {

namespace common {

Config Config::From(const std::string &config_str) {
    Config config;
    config.other_endpoints_.clear();
    const static std::regex reg(R"((\d{1,3}(\.\d{1,3}){3}:\d+))");
    std::sregex_iterator pos(config_str.begin(), config_str.end(), reg);
    decltype(pos) end;
    std::set<Endpoint, Endpoint> end_point_set;
    for (; pos != end; ++pos) {
        end_point_set.emplace(pos->str());
    }

    int64_t curr_id = 1;
    for (auto &end_point : end_point_set) {
        if (curr_id == 1) {
            config.this_endpoint_ = std::make_pair(curr_id++, end_point);
        } else {
            config.PushBack(curr_id++, end_point);
        }
    }
    return config;
}

}  // namespace common
}  // namespace raftcpp
