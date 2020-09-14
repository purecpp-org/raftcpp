
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include "common/id.h"

TEST_CASE("TestNodeid") {
    raftcpp::Endpoint ep("127.0.0.1:5000");
    raftcpp::NodeID id(ep);

    raftcpp::Endpoint ep2("127.0.0.1:5000");
    raftcpp::NodeID id2(ep2);
    REQUIRE_EQ(id, id2);

    raftcpp::Endpoint ep3("192.168.0.1:8080");
    raftcpp::NodeID id3(ep3);
    REQUIRE_NE(id2, id3);
}
