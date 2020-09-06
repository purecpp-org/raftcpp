
#include "common/timer.h"

#include <asio/io_service.hpp>
#include <iostream>
#include <thread>

#include "common/util.h"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

TEST_CASE("test_repeated_timer_reset") {
    using namespace raftcpp::common;

    asio::io_service io_service;
    asio::io_service::work work{io_service};

    uint64_t callback_invoked_time_ms = -1;
    RepeatedTimer repeated_timer{io_service,
                                 [&callback_invoked_time_ms](const asio::error_code &e) {
                                     callback_invoked_time_ms = CurrentTimeMs();
                                 }};
    std::thread th{[&io_service]() {
        io_service.run();
    }};

    repeated_timer.Start(3 * 1000);
    std::this_thread::sleep_for(std::chrono::milliseconds{500});

    repeated_timer.Reset(6 * 1000);
    auto resetting_time_ms = CurrentTimeMs();
    REQUIRE_EQ(true, callback_invoked_time_ms > 0);
    REQUIRE_EQ(true, (callback_invoked_time_ms - resetting_time_ms > 0));

    repeated_timer.Stop();
    io_service.stop();
    th.join();
}

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
