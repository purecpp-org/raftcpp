#include "../src/common/config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("config-tset") {
    using namespace raftcpp::common;

    Config config = Config::From("");
    REQUIRE_EQ(config.GetOtherEndpoints().size(), 0);

    config = Config::From("123");
    REQUIRE_EQ(config.GetOtherEndpoints().size(), 0);

    config = Config::From("127.0.0.1:10001");
    REQUIRE_EQ(config.GetOtherEndpoints().size(), 0);
    REQUIRE_EQ(config.GetThisEndpoint().GetHost(), "127.0.0.1");
    REQUIRE_EQ(config.GetThisEndpoint().GetPort(), 10001);

    config = Config::From("127.0.0.1:10001,127.0.0.1:10002");
    REQUIRE_EQ(config.GetOtherEndpoints().size(), 1);

    config = Config::From("127.0.0.1:10001,127.0.0.1:10002,");
    REQUIRE_EQ(config.GetOtherEndpoints().size(), 1);

    config = Config::From("127.0.0.1:10001,127.0.0.1:10002,123");
    REQUIRE_EQ(config.GetOtherEndpoints().size(), 1);

    config = Config::From(
        "127.0.0.1:10001,127.0.0.1:10002,255.255.255.255:10003,0.0.0.0:10004");
    REQUIRE_EQ(config.GetOtherEndpoints().size(), 3);
}
