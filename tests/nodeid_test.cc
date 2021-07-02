
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include <unordered_map>

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

    raftcpp::Endpoint ep4("192.168.0.1:8081");
    raftcpp::NodeID id4(ep4);

    std::unordered_map<raftcpp::NodeID, bool> um;
    um.insert(std::pair<raftcpp::NodeID, bool>{id, true});
    um.insert(std::pair<raftcpp::NodeID, bool>{id2, true});
    um.insert(std::pair<raftcpp::NodeID, bool>{id3, true});

    REQUIRE_EQ(2, um.size());
    REQUIRE_NE(um.cend(), um.find(id));
    REQUIRE_NE(um.cend(), um.find(id2));
    REQUIRE_NE(um.cend(), um.find(id3));
    REQUIRE_EQ(um.cend(), um.find(id4));
}

TEST_CASE("TestTermid") {
    raftcpp::TermID id(1);
    raftcpp::TermID id2(1);
    REQUIRE_EQ(id, id2);

    raftcpp::TermID id3(3);
    REQUIRE_NE(id2, id3);
    REQUIRE_EQ(3, id3.getTerm());

    id2.setTerm(3);
    REQUIRE_EQ(id2, id3);

    raftcpp::TermID id4(4);

    std::unordered_map<raftcpp::TermID, bool> um;
    um.insert(std::pair<raftcpp::TermID, bool>{id, true});
    um.insert(std::pair<raftcpp::TermID, bool>{id2, true});
    um.insert(std::pair<raftcpp::TermID, bool>{id3, true});

    REQUIRE_EQ(2, um.size());
    REQUIRE_NE(um.cend(), um.find(id));
    REQUIRE_NE(um.cend(), um.find(id2));
    REQUIRE_NE(um.cend(), um.find(id3));
    REQUIRE_EQ(um.cend(), um.find(id4));
}
