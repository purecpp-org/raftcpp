#pragma once

#include "util.h"
#include <random>
namespace raftcpp {
    class Randomer {
    public:
        Randomer() {
            e.seed(common::CurrentTimeUs());
        }

        Randomer(const Randomer &rd) = delete;

        Randomer &operator=(const Randomer &rd) = delete;

        uint64_t TakeOne(const uint64_t begin, const uint64_t end) {
            u.param(std::uniform_int_distribution<uint64_t>::param_type{begin, end});
            return u(e);
        }

    private:
        std::default_random_engine e;
        std::uniform_int_distribution<uint64_t> u;
    };
}