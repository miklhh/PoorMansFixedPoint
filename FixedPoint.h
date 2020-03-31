/*
 * PoorMansFixedPoint header only C++ fixed point module. It supports fixed
 * point numbers of varying length and the most basic arithmetic functions
 * +,-,*,/ with proper rounding.
 *
 * Author: Mikael Henriksson
 */

#ifndef _POOR_MANS_FIXED_POINT_H
#define _POOR_MANS_FIXED_POINT_H

#include <ostream>
#include <string>
#include <cmath>

/*
 * Debuging stuff for being able to display over-/underflows.
 */
#ifdef _DEBUG_SHOW_OVERFLOW_INFO
    #include <sstream>

    /*
     * If using this debuging functionallity, define in this function how an
     * output string should be passed to the user.
     */
    #include <iostream>
    void _DEBUG_PRINT_FUNC(const char *str)
    {
        // Example debug feed forward.
        std::cerr << str << std::endl;
    }

#endif

template <int INT_BITS, int FRAC_BITS, int NODE_ID = 0>
class FixedPoint
{
    static_assert(INT_BITS <= 32, "Integer bits need to be less than or equal to 32 bits.");
    static_assert(FRAC_BITS <= 32, "Fractional bits need to be less than or equal to 32 bits.");
    static_assert(INT_BITS + FRAC_BITS > 0, "Need at least one bit of representation.");

protected:
    /*
     * Long long is guaranteed to be atleast 64-bits wide. We use the 32 most
     * significant bits to store the integer part and the 32 least significant
     * bits to store the fraction.
     */
    long long num{};

    /*
     * Friend declaration for accessing 'num' between different types, i.e,
     * between template instances with different wordlenth.
     */
    template <int _INT_BITS, int _FRAC_BITS, int _NODE_ID>
    friend class FixedPoint;

    /*
     * Friend declaration for stream output method.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    friend std::ostream &
        operator<<(
                std::ostream &os,
                const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS, RHS_NODE_ID> &rhs);


    /*
     * Private rounding function. A note on the usage of this method, it is
     * virtual such that a debug class can override it, in which over-/underflow
     * can be detected and displayed to the user. For usage on 'oneself', invoke
     * this method 'this->round(this->num)' to properly round and apply the
     * bitmask.
     */
    virtual void round(long long &num, bool is_division = false) const noexcept
    {
        if (FRAC_BITS < 32)
            num += 1ll << (31-FRAC_BITS);

        /*
         * Mute warning of unused parameter is_division which is only used in
         * the debuging state.
         */
        (void) is_division;

#ifdef _DEBUG_SHOW_OVERFLOW_INFO
        /*
         * If enebaled, test for over-/underflow for the result and present user
         * with a warning.
         */
        if ( !is_division && test_over_or_underflow(num) )
        {
            std::stringstream ss{};
            ss << (((1ull << 63) & num) ? "Underflow " : "Overflow ");
            ss << "detected in node with id: '" << NODE_ID << "' with value: ";
            ss << (num >> 32) << " + " << this->get_frac_quotient(num);

            num &= ((1ll << (INT_BITS+FRAC_BITS)) - 1) << (32-FRAC_BITS);
            ss << ". Truncated to: " << (this->get_num_sign_extended(num) >> 32);
            ss << " + " << this->get_frac_quotient(num);
            _DEBUG_PRINT_FUNC(ss.str().c_str());
        }
#endif

        // Apply the bit mask.
        num &= ((1ll << (INT_BITS+FRAC_BITS)) - 1) << (32 - FRAC_BITS);
    }

    /*
     * Quick round function for rounding oneself. This method is short hand for
     * invoking 'this->round(this->num)'.
     */
    void round(bool is_division = false) noexcept
    {
        this->round(this->num, is_division);
    }

    /*
     * Get the current number (or another number, see overloads!) sign extended
     * to Q(32, 32) format.
     */
    long long get_num_sign_extended(long long num) const noexcept
    {
        /*
         * Instead of of testing if we need to sign extend the internal number
         * num, we left shift it (unsigned, logically) all the way to the MSb,
         * and then shift it (signed, arithmeticaly) back to its original
         * position. This has performance benifits. Note that right shift of a
         * signed value is implementation defined, but that both GCC and CLANG
         * seems to do arithmetical shifts, as intended, for tested systems.
         */
        using uns_ll = unsigned long long;
        uns_ll l = static_cast<uns_ll>(num) << (32-INT_BITS);
        return static_cast<long long>(l) >> (32-INT_BITS);
    }
    long long get_num_sign_extended() const noexcept
    {
        return get_num_sign_extended(this->num);
    }

    /*
     * Private method for testing over/underflow.
     */
    bool test_over_or_underflow(long long &num) const noexcept
    {
        // Implementation defined behaviour. We expect the result of signed
        // right shift to be arithmetic shift (sign extending). This holds true
        // for GCC and CLANG on tested machines. Test it yourself by running the
        // unit test.
        unsigned long long msb_extended = (num >> (31+INT_BITS));
        return !( (msb_extended == -1ull) || (msb_extended == 0ull) );
    }
    bool test_over_or_underflow() const noexcept
    {
        return test_over_or_underflow(this->num);
    }

public:
    FixedPoint() = default;
    virtual ~FixedPoint() = default;

    /*
     * Constructor for initialization from other fixed point number. Please note
     * that usage of this constructor can cause data spill if the number stored
     * in the RHS is to big for that of the LHS. If the RHS number does not fit
     * in LHS one the overflowing bits are lost.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    FixedPoint(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        noexcept
    {
        this->num = rhs.get_num_sign_extended();
        this->round();
    }
    FixedPoint(const FixedPoint<INT_BITS, FRAC_BITS> &rhs) noexcept
    {
        this->num = rhs.num;
    }

    /*
     * Constructor for floating point number inputs.
     */
    FixedPoint(double a)
    {
        this->num = std::llround(a * static_cast<double>(1ll << 32));
        this->round();
    }

    /*
     * Constructor for integers.
     */
    FixedPoint(int n)
    {
        this->num = static_cast<long long>(n) << 32;
        this->round();
    }

    /*
     * Constructor for manually setting bot int and frac part.
     */
    FixedPoint(int i, unsigned f) noexcept
    {
        this->num = (static_cast<long long>(i) << 32) | (0xFFFFFFFFll & num);
        this->num &= 0xFFFFFFFF00000000ll;
        this->num |= 0xFFFFFFFFll & (static_cast<long long>(f) << (32 - FRAC_BITS));
        this->round();
    }

    /*
     * Basic getters and setters.
     */
    int get_int_bits() const noexcept { return INT_BITS; }
    int get_frac_bits() const noexcept { return FRAC_BITS; }
    int get_int() const noexcept { return static_cast<int>(num >> 32); }
    int get_frac() const noexcept { return static_cast<int>(num & 0xFFFFFFFFll); }

    /*
     * Retrieve a string of the fractional part of the FixedPoint number, useful
     * for displaying the FixedPoint number content.
     */
    std::string get_frac_quotient(const long long &num) const noexcept
    {
        std::string numerator = std::to_string((num & 0xFFFFFFFF) >> (32-FRAC_BITS));
        std::string denominator = std::to_string(1ll << FRAC_BITS);
        return numerator + "/" + denominator;
    }
    std::string get_frac_quotient() const noexcept
    {
        return get_frac_quotient(this->num);
    }

    /*
     * Assigment operators of FixedPoint numbers.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    FixedPoint<INT_BITS, FRAC_BITS, NODE_ID> &
        operator=(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        noexcept
    {
        this->num = rhs.get_num_sign_extended();
        this->round();
        return *this;
    }
    FixedPoint<INT_BITS, FRAC_BITS> &
        operator=(const FixedPoint<INT_BITS, FRAC_BITS> &rhs) noexcept
    {
        this->num = rhs.num;
        return *this;
    }
    FixedPoint<INT_BITS, FRAC_BITS> &
        operator=(int rhs) noexcept
    {
        this->num = static_cast<long long>(rhs) << 32;
        this->round();
        return *this;
    }

    /*
     * (explicit) Conversion to floating point number.
     */
    explicit operator double() const noexcept
    {
        // Test if sign extension is needed.
        return static_cast<double>(this->get_num_sign_extended()) /
               static_cast<double>(1ll << 32);
    }

    /*
     * Unary negation operator.
     */
    FixedPoint<INT_BITS, FRAC_BITS> operator-() const noexcept
    {
        FixedPoint<INT_BITS, FRAC_BITS> res{};
        res.num = -( this->get_num_sign_extended() );
        this->round(res.num);
        return res;
    }

    /*
     * Addition/subtraction of FixedPoint numbers. Result will have word length
     * equal to that of the left hand side of the operator.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    FixedPoint<INT_BITS, FRAC_BITS, NODE_ID>
        operator+(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS,RHS_NODE_ID> &rhs)
        const noexcept
    {
        FixedPoint<INT_BITS, FRAC_BITS, NODE_ID> res;
        res.num = this->get_num_sign_extended() + rhs.get_num_sign_extended();
        this->round(res.num);
        return res;
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    FixedPoint<INT_BITS, FRAC_BITS, NODE_ID> &
        operator+=(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS,RHS_NODE_ID> &rhs)
        noexcept
    {
        this->num = this->get_num_sign_extended() + rhs.get_num_sign_extended();
        this->round();
        return *this;
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    FixedPoint<INT_BITS, FRAC_BITS, NODE_ID>
        operator-(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        const noexcept
    {
        FixedPoint<INT_BITS, FRAC_BITS> res;
        res.num = this->get_num_sign_extended() - rhs.get_num_sign_extended();
        this->round(res.num);
        return res;
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    FixedPoint<INT_BITS, FRAC_BITS, NODE_ID> &
        operator-=(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        noexcept
    {
        this->num = this->get_num_sign_extended() - rhs.get_num_sign_extended();
        this->round();
        return *this;
    }

    /*
     * Multilication of FixedPoint numbers.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    FixedPoint<INT_BITS, FRAC_BITS, NODE_ID>
        operator*(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        const noexcept
    {
        /*
         * Scenario 1:
         * The entire result of the multiplication can fit into one 64-bit
         * integer. This code produces faster result when applicable.
         */
        if (INT_BITS+FRAC_BITS <= 32 && RHS_INT_BITS+RHS_FRAC_BITS <= 32)
        {
            FixedPoint<INT_BITS, FRAC_BITS> res{};
            long long op_a{ this->get_num_sign_extended() >> INT_BITS     };
            long long op_b{   rhs.get_num_sign_extended() >> RHS_INT_BITS };
            res.num = op_a * op_b;

            // Shift result to the correct place.
            if (INT_BITS + RHS_INT_BITS > 32)
            {
                res.num <<= INT_BITS + RHS_INT_BITS - 32;
            }
            else
            {
                res.num >>= 32 - INT_BITS - RHS_INT_BITS;
            }
            this->round(res.num);
            return res;
        }
        /*
         * Scenario 2:
         * The entire result of the multiplication can fit into one 128-bit
         * integer. Running this code takes a little longer time than running
         * the code of scenario 1, probably due to the fact that this code won't
         * be accelerated by any integer vectorization. However, this piece of
         * code seems to work for all sizes of FixedPoints.
         */
        else
        {
            // Utilize the compiler extension of 128-bit wide integers to be
            // able to store the exact result.
            FixedPoint<INT_BITS, FRAC_BITS> res{};
            __extension__ __int128 op_a{ this->get_num_sign_extended() };
            __extension__ __int128 op_b{   rhs.get_num_sign_extended() };
            __extension__ __int128 res_128{ op_a * op_b };

            // Shift result back to the form Q(32,32) and return result.
            res.num = static_cast<long long>(res_128 >> 32);
            this->round(res.num);
            return res;
        }
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    FixedPoint<INT_BITS, FRAC_BITS, NODE_ID> &
        operator*=(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        noexcept
    {
        FixedPoint<INT_BITS, FRAC_BITS, NODE_ID> res{ *this * rhs };
        this->num = res.num;
        return *this;
    }

    /*
     * Division of FixedPoint numbers with integers. Result will have word
     * length equal to that of the left hand side of the operator, but the
     * precision of the result will not necessary represent such a wide number.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    FixedPoint<INT_BITS, FRAC_BITS>
        operator/(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        const
    {
        // Note that Q(a,64) / Q(b,32) == Q(c,32).
        FixedPoint<INT_BITS, FRAC_BITS, NODE_ID> res{};
        __extension__ __int128 dividend{ this->get_num_sign_extended() };
        __extension__ __int128 divisor {   rhs.get_num_sign_extended() };
        dividend <<= 32;

        // Create and return result. Note that Q(a,64) / Q(b,32) == Q(c,32).
        res.num = static_cast<long long>( dividend/divisor );
        this->round(res.num, true);
        return res;
    }
    FixedPoint<INT_BITS, FRAC_BITS, NODE_ID>
        operator/(int rhs) const
    {
        FixedPoint<INT_BITS, FRAC_BITS, NODE_ID> res;
        res.num = this->num / rhs;
        this->round(res.num, true);
        return res;
    }
    FixedPoint<INT_BITS, FRAC_BITS, NODE_ID> &
        operator/=(int rhs)
    {
        this->num /= rhs;
        this->round(true);
        return *this;
    }

    /*
     * Comparison operators.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    bool operator==(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        const noexcept
    {
        return this->get_num_sign_extended() == rhs.get_num_sign_extended();
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    bool operator!=(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        const noexcept
    {
        return this->get_num_sign_extended() != rhs.get_num_sign_extended();
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    bool operator<(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        const noexcept
    {
        return this->get_num_sign_extended() < rhs.get_num_sign_extended();
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    bool operator<=(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        const noexcept
    {
        return this->get_num_sign_extended() <= rhs.get_num_sign_extended();
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    bool operator>(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS,RHS_NODE_ID> &rhs)
        const noexcept
    {
        return this->get_num_sign_extended() > rhs.get_num_sign_extended();
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS, int RHS_NODE_ID>
    bool operator>=(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS, RHS_NODE_ID> &rhs)
        const noexcept
    {
        return this->get_num_sign_extended() >= rhs.get_num_sign_extended();
    }
};


/*
 * Print-out to C++ stream object on the form '<int> + <frac>/<2^<frac_bits>'.
 * Good for debuging'n'stuff.
 */
template <int INT_BITS, int FRAC_BITS, int NODE_ID>
std::ostream &operator<<(std::ostream &os, const FixedPoint<INT_BITS, FRAC_BITS, NODE_ID> &rhs)
{
    long long num{ rhs.get_num_sign_extended() };
    return os << (num >> 32) << " + " << rhs.get_frac_quotient();
}

/*
 * Header guard end if.
 */
#endif
