#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest.h>

#include "util.h"

TEST_CASE("test_cluster") {
    Cluster clu(3);
    std::this_thread::sleep_for(std::chrono::seconds(10));
}