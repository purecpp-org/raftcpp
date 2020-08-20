
#include "flags.h"

namespace raftcpp::common {

inline uint32_t FlagValueToInt(const std::string &flag_value) {
    // throw exception
    return static_cast<uint32_t>(std::stoi(flag_value));
}

inline std::string TrimFlagName(const std::string &flag_name_with_prefix) {
    // throw exception
    if (flag_name_with_prefix.empty() || flag_name_with_prefix.size() == 1) { return ""; }

    if (flag_name_with_prefix[0] == '-' && flag_name_with_prefix[1] == '-') {
        return flag_name_with_prefix.substr(2, flag_name_with_prefix.size());
    } else if (flag_name_with_prefix[0] == '-') {
        return flag_name_with_prefix.substr(1, flag_name_with_prefix.size());
    } else {
        return "";
    }
}

Flags::Flags(int argc, char *argv[]) :argc_(argc) {
    if (argc < 1) { return; }

    app_name_ = argv[0];
    for (int i = 1; i < argc && i + 1 < argc; i += 2) {
        const std::string flag_name = TrimFlagName(argv[i]);
        const std::string flag_value = argv[i + 1];
        flags_[flag_name] = flag_value;
    }
}

uint32_t Flags::FlagInt(const std::string &flag_name, const uint32_t default_value) {
    auto it = flags_.find(flag_name);
    if (it == flags_.end()) {
        return default_value;
    }

    return FlagValueToInt(it->second);
}

std::string Flags::FlagStr(const std::string &flag_name, const std::string &default_value) {
    auto it = flags_.find(flag_name);
    if (it == flags_.end()) {
        return default_value;
    }

    return it->second;
}

}
