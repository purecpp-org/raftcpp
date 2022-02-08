#include "../src/common/config.h"

#include <fstream>
#include <unordered_map>

#include "gtest/gtest.h"

TEST(ConfigTest, TestBasicConfig) {
    using namespace raftcpp::common;

    Config config = Config::From("");
    ASSERT_EQ(config.GetOtherEndpoints().size(), 0);

    config = Config::From("123");
    ASSERT_EQ(config.GetOtherEndpoints().size(), 0);

    config = Config::From("127.0.0.1:10001");
    ASSERT_EQ(config.GetOtherEndpoints().size(), 0);
    ASSERT_EQ(config.GetThisEndpoint().GetHost(), "127.0.0.1");
    ASSERT_EQ(config.GetThisEndpoint().GetPort(), 10001);

    config = Config::From("127.0.0.1:10001,127.0.0.1:10002");
    ASSERT_EQ(config.GetOtherEndpoints().size(), 1);

    config = Config::From("127.0.0.1:10001,127.0.0.1:10002,");
    ASSERT_EQ(config.GetOtherEndpoints().size(), 1);

    config = Config::From("127.0.0.1:10001,127.0.0.1:10002,123");
    ASSERT_EQ(config.GetOtherEndpoints().size(), 1);

    config = Config::From(
        "127.0.0.1:10001,127.0.0.1:10002,255.255.255.255:10003,0.0.0.0:10004");
    ASSERT_EQ(config.GetOtherEndpoints().size(), 3);

    std::string input = "0.0.0.0:0,0.0.0.0:1,255.255.255.255:65534,255.255.255.255:65535";
    config = Config::From(input);
    std::string output = config.ToString();
    ASSERT_EQ(input, output);
    ASSERT_EQ(config, config);

    input = "0.0.0.0:0,255.255.255.255:65534,255.255.255.255:65535,0.0.0.0:1";
    Config config2 = Config::From(input);
    ASSERT_EQ(config, config2);
    output = config2.ToString();
    ASSERT_EQ(input, output);

    input = "0.0.0.0:0,0.0.0.0:1,255.255.255.255:65534,255.255.255.255:65533";
    config2 = Config::From(input);
    ASSERT_NE(config, config2);

    input = "0.0.0.0:65535";
    config2 = Config::From(input);
    ASSERT_NE(config, config2);
}

TEST(ConfigTest, TestEndpoint) {
    using namespace raftcpp;

    Endpoint e0("127.0.0.1:10001");
    Endpoint e1("127.0.0.1:10001");
    Endpoint e2("0.0.0.0:0");
    Endpoint e3("255.255.255.255:65535");
    Endpoint e4("255.255.255.255:65534");

    std::vector<Endpoint> v{e0, e1, e2, e3, e4};
    ASSERT_EQ(e0, e1);
    ASSERT_NE(e3, e4);

    std::sort(v.begin(), v.end(), Endpoint());

    ASSERT_EQ(v[0], e2);
    ASSERT_EQ(v[1], e0);
    ASSERT_EQ(v[2], e1);
    ASSERT_EQ(v[3], e4);
    ASSERT_EQ(v[4], e3);

    std::unordered_map<Endpoint, std::string> m = {{{"127.0.0.1", 1001}, "first"},
                                                   {{"127.0.0.1", 1002}, "second"}};
    std::string key1 = "127.0.0.1:1001";
    std::string key2 = "127.0.0.1:1002";
    for (const auto &element : m) {
        std::stringstream buffer;
        buffer << element.first;
        if (element.second == "first") {
            ASSERT_EQ(buffer.str(), key1);
        } else if (element.second == "first") {
            ASSERT_EQ(buffer.str(), key2);
        }
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
