//
// Created by think on 2020/8/4.
//

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

int factorial(int number) { return number <= 1 ? number : factorial(number - 1) * number; }

TEST_CASE("testing the factorial function") {
CHECK(factorial(1) == 1);
CHECK(factorial(2) == 2);
CHECK(factorial(3) == 6);
CHECK(factorial(10) == 3628800);
}