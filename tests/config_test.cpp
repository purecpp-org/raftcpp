#include "../src/common/config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("config-tset") {
    using namespace raftcpp;

    Config config = Config::From("");
    REQUIRE_EQ(config.GetAllEndpoints().size(), 0);

    config = Config::From("123");
    REQUIRE_EQ(config.GetAllEndpoints().size(), 0);

    config = Config::From("127.0.0.1:10001");
    REQUIRE_EQ(config.GetAllEndpoints().size(), 1);

    config = Config::From("127.0.0.1:10001,127.0.0.1:10002");
    REQUIRE_EQ(config.GetAllEndpoints().size(), 2);

    config = Config::From("127.0.0.1:10001,127.0.0.1:10002,");
    REQUIRE_EQ(config.GetAllEndpoints().size(), 2);

    config = Config::From("127.0.0.1:10001,127.0.0.1:10002,123");
    REQUIRE_EQ(config.GetAllEndpoints().size(), 2);

    config = Config::From(
        "127.0.0.1:10001,127.0.0.1:10002,255.255.255.255:10003,0.0.0.0:10004");
    REQUIRE_EQ(config.GetAllEndpoints().size(), 4);
}
