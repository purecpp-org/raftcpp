#include "common/config.h"

namespace raftcpp {

namespace common {

Config Config::From(const std::string &config_str) {
    Config config;
    config.endpoints.clear();
    const static std::regex reg(R"((\d{1,3}(\.\d{1,3}){3}:\d+))");
    std::sregex_iterator pos(config_str.begin(), config_str.end(), reg);
    decltype(pos) end;
    for (; pos != end; ++pos) {
        config.PushBack(Endpoint(pos->str()));
    }
    return config;
}

}  // namespace common
}  // namespace raftcpp
