
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include "common/logging.h"

TEST_CASE("TestLogCheck") {
    RAFTCPP_CHECK(true);
    RAFTCPP_CHECK(false);
}
