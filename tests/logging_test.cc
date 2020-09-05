
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

// nanolog INFO (WARN) and doctest INFO (WARN) conflict
#ifdef INFO
#undef INFO
#endif

#ifdef WARN
#undef WARN
#endif

#include "common/logging.h"

TEST_CASE("logging_test") {
    using namespace raftcpp;
    RAFTCPP_LOG(INFO) << "Hello " << 123;
}
