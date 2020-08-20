
#include "common/flags.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>


TEST_CASE("flags_test") {
    using namespace raftcpp::common;
    char *argv[] = {"MyTestApp", "--addr", "localhost"};
    Flags flags(3, argv);
    const auto addr = flags.FlagStr("addr");
    REQUIRE_EQ("localhost", addr);
}
