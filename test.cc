#include "catch.hpp"
#include "FixedPoint.h"
#include <sstream>

TEST_CASE("Template arguments")
{
    FixedPoint<5,3> fix_a;
    REQUIRE(fix_a.get_int_bits() == 5);
    REQUIRE(fix_a.get_frac_bits() == 3);
}

TEST_CASE("Floating-point constructor")
{
    std::stringstream result{};
    FixedPoint<10,10> fix_a{ 3.25 };
    result << fix_a;
    REQUIRE(result.str() == std::string("3 + 256/1024"));
}

TEST_CASE("Addition")
{
    std::stringstream result{};
    FixedPoint<10,10> fix_a{3.25};
    FixedPoint<11,11> fix_b{7.50};
    result << fix_a + fix_b;
    REQUIRE(result.str() == std::string("10 + 768/1024"));
}
