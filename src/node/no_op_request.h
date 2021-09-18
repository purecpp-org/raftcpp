#pragma once

#include <string>

#include "rpc/common.h"

namespace raftcpp::node {

class NoOpRequest : public RaftcppRequest {
public:
    std::string Serialize() override { return ""; };

    void Deserialize(const std::string &s) override{};
};

}  // namespace raftcpp::node
