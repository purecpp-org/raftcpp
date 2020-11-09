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

    std::string input = "0.0.0.0:0,0.0.0.0:1,255.255.255.255:65534,255.255.255.255:65535";
    config = Config::From(input);
    std::string output = config.ToString();
    REQUIRE_EQ(input, output);
    REQUIRE_EQ(config, config);

    input = "0.0.0.0:0,255.255.255.255:65534,255.255.255.255:65535,0.0.0.0:1";
    Config config2 = Config::From(input);
    REQUIRE_EQ(config, config2);
    output = config2.ToString();
    REQUIRE_EQ(input, output);

    input = "0.0.0.0:0,0.0.0.0:1,255.255.255.255:65534,255.255.255.255:65533";
    config2 = Config::From(input);
    REQUIRE_NE(config, config2);

    input = "0.0.0.0:65535";
    config2 = Config::From(input);
    REQUIRE_NE(config, config2);
}

TEST_CASE("endpoint-test") {
    using namespace raftcpp;

    Endpoint e0("127.0.0.1:10001");
    Endpoint e1("127.0.0.1:10001");
    Endpoint e2("0.0.0.0:0");
    Endpoint e3("255.255.255.255:65535");
    Endpoint e4("255.255.255.255:65534");

    std::vector<Endpoint> v{e0, e1, e2, e3, e4};
    REQUIRE_EQ(e0, e1);
    REQUIRE_NE(e3, e4);

    std::sort(v.begin(), v.end(), Endpoint());

    REQUIRE_EQ(v[0], e2);
    REQUIRE_EQ(v[1], e0);
    REQUIRE_EQ(v[2], e1);
    REQUIRE_EQ(v[3], e4);
    REQUIRE_EQ(v[4], e3);
}
