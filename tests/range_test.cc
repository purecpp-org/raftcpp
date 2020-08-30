
#include "common/range.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

TEST_CASE("Range-BasicTest") {
    using namespace raftcpp::common;

    Range range(10, 100);
    REQUIRE_EQ(10, range.GetBegin());
    REQUIRE_EQ(100, range.GetEnd());
    REQUIRE_EQ(90, range.GetDelta());
}
