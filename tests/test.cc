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
    /*
     * Two simple tests.
     */
    {
        std::stringstream result{};
        FixedPoint<10,10> fix_a{ 3.25 }, fix_b{ -19.125 };
        result << fix_a << "|" << fix_b;
        REQUIRE(result.str() == std::string("3 + 256/1024|-20 + 896/1024"));
    }

    /*
     * More tests.
     */
    {
        std::stringstream result{};
        FixedPoint<8,8> fix_a{ -1.55555555 }, fix_b{ -0.555555555 };
        result << fix_a << "|" << fix_b;
        REQUIRE(result.str() == std::string("-2 + 114/256|-1 + 114/256"));
    }

    /*
     * Test Zero.
     */
    {
        std::stringstream result{};
        FixedPoint<12,12> fix_a{ 0.0 };
        result << fix_a;
        REQUIRE(result.str() == std::string("0 + 0/4096"));
    }

    /*
     * Test correct rounding close to zero.
     */
    {
        std::stringstream result{};
        FixedPoint<12,12> fix_a{ -0.0001 }, fix_b{ -0.0002 };
        result << fix_a << "|" << fix_b;
        REQUIRE(result.str() == std::string("0 + 0/4096|-1 + 4095/4096"));
    }

}

TEST_CASE("Addition")
{
    std::stringstream result{};
    FixedPoint<10,10> fix_a{3.25};
    FixedPoint<11,11> fix_b{7.50};
    result << fix_a + fix_b;
    REQUIRE(result.str() == std::string("10 + 768/1024"));
}
