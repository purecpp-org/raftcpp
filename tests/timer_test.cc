
#include "common/timer.h"

#include <asio/io_service.hpp>
#include <iostream>
#include <thread>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>


TEST_CASE("Timer-ContinuousTimer") {
    using namespace raftcpp::common;

    uint64_t counter = 0;

    asio::io_service io_service;
    ContinuousTimer continuous_timer(
        io_service, 1000, [&counter](const asio::error_code &e) { ++counter; });
    continuous_timer.Start();
    std::thread th([&io_service]() { io_service.run(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(5 * 1000 + 600));

    continuous_timer.Cancel();
    // Note that stop a io_service is thread safe.
    io_service.stop();
    th.join();

    REQUIRE_EQ(5, counter);
}
