#include "catch.hpp"
#include "FixedPoint.h"
#include <sstream>
#include <iostream>
#include <cmath>


TEST_CASE("Template arguments")
{
    FixedPoint<5,3> fix_a;
    REQUIRE(fix_a.get_int_bits() == 5);
    REQUIRE(fix_a.get_frac_bits() == 3);
}

TEST_CASE("Instance going out of scope should reset value.")
{
    for (int i=0; i<5; ++i)
    {
        FixedPoint<10,12> fix{ 0.0 };
        fix += FixedPoint<10,12>(i);
        fix /= 2;
        REQUIRE( static_cast<double>(fix) == static_cast<double>(i) / 2 );
    }
}

TEST_CASE("Assigment of FixedPoint values (just need to compile).")
{
    FixedPoint<10,10> fix_a{}, fix_b{};
    FixedPoint<10,10> fix_c = fix_a * fix_b;
    fix_c = fix_a * fix_b;

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

TEST_CASE("Fixed point to floating point conversion introductory test.")
{
    FixedPoint<6,10> fix_a{ -5.25 };
    FixedPoint<9,16> fix_b{ 2.33 };
    REQUIRE(static_cast<double>(fix_a) == -5.25);
    REQUIRE(std::abs(static_cast<double>(fix_b) -2.33) < 0.0001);
}


TEST_CASE("Addition")
{
    {
        std::stringstream result{};
        FixedPoint<10,10> fix_a{3.25};
        FixedPoint<11,11> fix_b{7.50};
        result << fix_a + fix_b;
        REQUIRE(result.str() == std::string("10 + 768/1024"));
    }
    {
        std::stringstream result{};
        FixedPoint<10,10> fix_a{3.3333333}; // (3.33301) when rounded.
        FixedPoint<10,10> fix_b{7.4444444}; // (4.44433) when rounded.
        result << fix_a + fix_b;
        REQUIRE(result.str() == std::string("10 + 796/1024"));
    }
}


TEST_CASE("Multiplication of Fixed Point Numbers")
{
    /*
     * Basic multiplication with (pos, pos), (pos, neg), (neg, pos) and (neg,
     * neg)
     */
    {
        std::stringstream result{};
        FixedPoint<10,10> fix_a = { 3.25 };
        FixedPoint<12,12> fix_b = { 1.925 };
        result << fix_b * fix_a;
        REQUIRE(result.str() == std::string("6 + 1050/4096"));
    }
    {
        std::stringstream result{};
        FixedPoint<10,10> fix_a = { -7.02 }; // (-7.01953) when rounded.
        FixedPoint<10,10> fix_b = { 1.925 }; // ( 1.92480) when rounded.
        result << fix_b * fix_a;
        REQUIRE(result.str() == std::string("-14 + 501/1024"));
    }
    {
        std::stringstream result{};
        FixedPoint<10,10> fix_a = { 3.25 };
        FixedPoint<12,12> fix_b = { -1.925 };
        result << fix_b * fix_a;
        REQUIRE(result.str() == std::string("-7 + 3046/4096"));
    }
    {
        std::stringstream result{};
        FixedPoint<10,10> fix_a = { -3.25 };
        FixedPoint<12,12> fix_b = { -1.925 };
        result << fix_b * fix_a;
        REQUIRE(result.str() == std::string("6 + 1050/4096"));
    }

    /*
     * Mulitplication with zero always equals zero.
     */
    {
        std::stringstream result{};
        FixedPoint<10,10> fix_a = { -3.25 };
        FixedPoint<12,12> fix_b = { 0 };
        result << fix_b * fix_a;
        REQUIRE(result.str() == std::string("0 + 0/4096"));
    }

    /*
     * Multiplication when INT_BITS+FRAC_BITS > 32.
     */
    {
        std::stringstream result{};
        FixedPoint<20,21> fix_a = { 1050.239 };
        FixedPoint<20,21> fix_b = { 238.052 };
        result << fix_b * fix_a;
        REQUIRE(result.str() == std::string("250011 + 1036913/2097152"));
    }

}

TEST_CASE("Fixed point division")
{
    /*
     * Simple introductory test.
     */
    {
        std::stringstream result{};
        FixedPoint<13,22> fix_a = { 7.60 };
        FixedPoint<14,17> fix_b = { 3.40 };
        result << fix_a/fix_b;
        REQUIRE(result.str() == std::string("2 + 986891/4194304"));
    }

    /*
     * Negative operands.
     */
    {
        std::stringstream result{};
        FixedPoint<6,23> fix_a = { -7.60 };
        FixedPoint<5,20> fix_b = { 3.40 };
        result << fix_a/fix_b;
        REQUIRE(result.str() == std::string("-3 + 6414816/8388608"));
    }
    {
        std::stringstream result{};
        FixedPoint<6,23> fix_a = { 7.60 };
        FixedPoint<5,20> fix_b = { -3.40 };
        result << fix_a/fix_b;
        REQUIRE(result.str() == std::string("-3 + 6414816/8388608"));
    }
    {
        std::stringstream result{};
        FixedPoint<10,23> fix_a = { -7.60 };
        FixedPoint<5,25> fix_b = { -3.40 };
        result << fix_a/fix_b;
        REQUIRE(result.str() == std::string("2 + 1973790/8388608"));
    }
}

TEST_CASE("Approximate pi using Leibniz formula")
{
    /*
     * 10 000 000 iterations of Leibniz formula should result in a number close
     * to pi, correct up to 7 decimals (including the 3) when correctly rounded.
     */
    const double pi = 3.1415926535;
    const int ITERATIONS=10000000;

    int divisor = 3;
    FixedPoint<3,32> pi_fixed{4.0};
    for (int i=0; i<ITERATIONS; ++i)
    {
        if (i % 2)
        {
            // Odd iteration.
            pi_fixed += FixedPoint<3,30>{4.0}/divisor;
        }
        else
        {
            // Even iteration.
            pi_fixed -= FixedPoint<3,30>{4.0}/divisor;
        }
        divisor += 2;
    }

    std::cout << std::endl;
    std::cout << "Result from Leibniz formula of " << ITERATIONS << " iterations:" << std::endl;
    std::cout.precision(9);
    std::cout << "    Fixed     (fixed form)   : " << pi_fixed << std::endl;
    std::cout << "    Fixed     (decimal form) : " << static_cast<double>(pi_fixed) << std::endl;
    std::cout << "    Reference (decimal form) : " << pi << std::endl << std::endl;

    // We can acquire around 7 significant digits using this method.
    REQUIRE(std::abs(static_cast<double>(pi_fixed) - pi) < 0.000001);
}


TEST_CASE("Approximate e with Bernoulli limit.")
{
    /*
     * A note on this approch. Since the sum of fractional bits of the left hand
     * side and the right hand side of the multiplication is greater than 32, we
     * are going to lose some precision at the for the performed product. There
     * is no point of taking this algorithm further then it is already taken, it
     * won't yield more significant digits.
     */
    const int ITERATIONS=25000;
    const double e = 2.71828183;
    FixedPoint<3,30> product_fixed{ 1.0 + 1.0/static_cast<double>(ITERATIONS) };
    FixedPoint<3,30> e_fixed{ 1.0 };
    for (int i=0; i<ITERATIONS; ++i)
    {
        e_fixed *= product_fixed;
    }
    std::cout << "Result from Bernoulli limit: n=" << ITERATIONS << std::endl;
    std::cout << "    Fixed     (fixed form)   : " << e_fixed << std::endl;
    std::cout << "    Fixed     (decimal form) : " << static_cast<double>(e_fixed) << std::endl;
    std::cout << "    Reference (decimal form) : " << e << std::endl << std::endl;

    // We can acquire around 5 significant digits using this method.
    REQUIRE(std::abs(static_cast<double>(e_fixed) - e) < 0.0001);

}

TEST_CASE("Conversion from BIG fixed point numbers to floating point conversion.")
{
    FixedPoint<29,29> a{ 178956970, 357913941 };
    double a_ref{ 178956970.66666666604 };
    double a_error{ std::abs(a_ref - static_cast<double>(a)) };

    FixedPoint<30,30> b{ 536870911, 178956970 };
    double b_ref{ 536870911.16666666604 };
    double b_error{ std::abs(b_ref - static_cast<double>(b)) };

    FixedPoint<31,31> c{ -1073741823, 195225801 };
    double c_ref{ -1073741822.9090909018 };
    double c_error{ std::abs(c_ref - static_cast<double>(c)) };

    std::cout << "Result from BIG fixed point to floating point conversion:" << std::endl;
    std::cout.width(41); std::cout << "Reference";
    std::cout.width(36); std::cout << "Fixed->Float      | Error" << std::endl;
    std::cout.precision(20);
    std::cout << "    FixedPoint<29,29>: "; std::cout.width(23);
    std::cout << a_ref; std::cout.width(23); std::cout << static_cast<double>(a);
    std::cout << " | "; std::cout.width(3); std::cout << a_error << std::endl;

    std::cout << "    FixedPoint<30,30>: "; std::cout.width(23);
    std::cout << b_ref; std::cout.width(23); std::cout << static_cast<double>(b);
    std::cout << " | "; std::cout.width(3); std::cout << b_error << std::endl;

    std::cout << "    FixedPoint<31,31>: "; std::cout.width(23);
    std::cout << c_ref; std::cout.width(23); std::cout << static_cast<double>(c);
    std::cout << " | "; std::cout.width(3); std::cout << c_error << std::endl;

    // Error should be very small.
    double err_tol = 0.000001;
    REQUIRE((a_error < err_tol && b_error < err_tol && c_error < err_tol));
}
