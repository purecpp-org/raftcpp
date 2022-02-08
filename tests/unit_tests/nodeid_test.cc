#include <unordered_map>

#include "common/id.h"
#include "gtest/gtest.h"

TEST(IdTest, TestNodeid) {
    raftcpp::Endpoint ep("127.0.0.1:5000");
    raftcpp::NodeID id(ep);

    raftcpp::Endpoint ep2("127.0.0.1:5000");
    raftcpp::NodeID id2(ep2);
    ASSERT_EQ(id, id2);

    raftcpp::Endpoint ep3("192.168.0.1:8080");
    raftcpp::NodeID id3(ep3);
    ASSERT_NE(id2, id3);

    raftcpp::Endpoint ep4("192.168.0.1:8081");
    raftcpp::NodeID id4(ep4);

    std::unordered_map<raftcpp::NodeID, bool> um;
    um.insert(std::pair<raftcpp::NodeID, bool>{id, true});
    um.insert(std::pair<raftcpp::NodeID, bool>{id2, true});
    um.insert(std::pair<raftcpp::NodeID, bool>{id3, true});

    ASSERT_EQ(2, um.size());
    ASSERT_NE(um.cend(), um.find(id));
    ASSERT_NE(um.cend(), um.find(id2));
    ASSERT_NE(um.cend(), um.find(id3));
    ASSERT_EQ(um.cend(), um.find(id4));
}

TEST(IdTest, TestTermid) {
    raftcpp::TermID id(1);
    raftcpp::TermID id2(1);
    ASSERT_EQ(id, id2);

    raftcpp::TermID id3(3);
    ASSERT_NE(id2, id3);
    ASSERT_EQ(3, id3.getTerm());

    id2.setTerm(3);
    ASSERT_EQ(id2, id3);

    raftcpp::TermID id4(4);

    std::unordered_map<raftcpp::TermID, bool> um;
    um.insert(std::pair<raftcpp::TermID, bool>{id, true});
    um.insert(std::pair<raftcpp::TermID, bool>{id2, true});
    um.insert(std::pair<raftcpp::TermID, bool>{id3, true});

    ASSERT_EQ(2, um.size());
    ASSERT_NE(um.cend(), um.find(id));
    ASSERT_NE(um.cend(), um.find(id2));
    ASSERT_NE(um.cend(), um.find(id3));
    ASSERT_EQ(um.cend(), um.find(id4));
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
