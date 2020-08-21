
#include "common/flags.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"


TEST_CASE("flags_test") {
    using namespace raftcpp::common;
    char *argv[] = {"MyTestApp", "--addr", "localhost", "--port", "8888"};
    Flags flags(5, argv);
    const auto addr = flags.FlagStr("addr");
    const auto port = flags.FlagInt("port");
    REQUIRE_EQ("localhost", addr);
    REQUIRE_EQ(8888, port);
}
