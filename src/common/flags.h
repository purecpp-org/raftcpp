#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace raftcpp::common {

class Flags final {
public:
    Flags(int argc, char *argv[]);

    uint32_t FlagInt(const std::string &flag_name, const uint32_t default_value = 0);

    std::string FlagStr(const std::string &flag_name, const std::string &default_value = "");

private:
    uint32_t argc_;

    std::vector<std::string> argv_;

    std::string app_name_;

    std::unordered_map<std::string, std::string> flags_;
};

}