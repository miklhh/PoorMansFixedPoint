/*
 * PoorMansFixedPoint header only C++ fixed point module. It supports fixed
 * point numbers of varying length and the most basic arithmetic functions
 * +,-,*,/ with proper rounding.
 *
 * Author: Mikael Henriksson
 */

#include <ostream>
#include <string>
#include <cmath>

template <int INT_BITS, int FRAC_BITS, void (*PRINT_FUNC)(std::string) = nullptr>
class FixedPoint
{
    static_assert(INT_BITS <= 32, "Integer bits need to be less than or equal to 32 bits.");
    static_assert(FRAC_BITS <= 32, "Fractional bits need to be less than or equal to 32 bits.");
    static_assert(INT_BITS + FRAC_BITS > 0, "Need at least one bit of representation.");

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
    template <int _INT_BITS, int _FRAC_BITS, void (*_PRINT_FUNC)(std::string)>
    friend class FixedPoint;

    /*
     * Friend declaration for stream output method.
     */
    template <int _INT_BITS, int _FRAC_BITS>
    friend std::ostream &operator<<(std::ostream &os, const FixedPoint<_INT_BITS, _FRAC_BITS> &rhs);

    /*
     * Private rounding function, for rounding before applying the shift mask
     * (when appropriate). Rounding without applying the mask afterwards causes
     * undefined behaviour.
     */
    void round() noexcept
    {
        const long long BIT_MASK = ((1ll << (INT_BITS+FRAC_BITS)) - 1) << (32 - FRAC_BITS);
        if (FRAC_BITS < 32)
        {
            if ( (1ll << (31-FRAC_BITS)) & num )
            {
                // Rounding needed.
                this->num += 1ll << (32-FRAC_BITS);
            }
        }

        // Apply the bit mask.
        this->num &= BIT_MASK;
    }

    /*
     * Get the current number sign extended.
     */
    long long get_num_sign_extended() const noexcept
    {
        // Test if sign bit is set.
        if ( this->num & (1ll << (31+INT_BITS)) )
        {
            // Sign extend.
            unsigned long long mask = ((1ull << (32+INT_BITS)) - 1);
            return static_cast<long long>(static_cast<unsigned long long>(num) | ~mask);
        }
        else
        {
            return this->num;
        }
    }

public:
    FixedPoint() = default;

    /*
     * Constructor for initialization from other fixed point number. Please note
     * that usage of this constructor can cause data spill if the number stored
     * in the RHS is to big for that of the LHS. If the RHS number does not fit
     * in LHS one can basically view it as undefined behaviour.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS> &rhs) noexcept
    {
        // Sign extend in case RHS has a shorter wordlength than the left hand side.
        this->num = rhs.get_num_sign_extended();
        this->round();
    }
    /*
     * Constructor for floating point number input.
     */
    FixedPoint(double a)
    {
        // Shifting a negative signed value is implementation defined: (ISO/IEC
        // 9899:1999 Section 6.5.7). For GNU g++ (9.2.0) and CLANG++ (8.0.1)
        // this solution seems to work.
        this->num = std::llround(a * static_cast<double>(1ll << 32));
        this->round();
    }

    /*
     * Constructor for manually setting bot int and frac part.
     */
    FixedPoint(int i, unsigned f) noexcept
    {
        this->set_int(i);
        this->set_frac(f);
    }

    /*
     * Basic getters and setters.
     */
    int get_int_bits() const noexcept { return INT_BITS; }
    int get_frac_bits() const noexcept { return FRAC_BITS; }
    int get_int() const noexcept { return static_cast<int>(num >> 32); }
    int get_frac() const noexcept { return static_cast<int>(num & 0xFFFFFFFFll); }
    void set_int(int i) noexcept
    {
        this->num = (static_cast<long long>(i) << 32) | (0xFFFFFFFFll & num);
        this->round();
    }
    void set_frac(unsigned f) noexcept
    {
        num &= 0xFFFFFFFF00000000ll;
        num |= 0xFFFFFFFFll & (static_cast<long long>(f) << (32 - FRAC_BITS));
        round();
    }
    std::string get_frac_quotient() const noexcept
    {
        std::string numerator = std::to_string((num & 0xFFFFFFFF) >> (32-FRAC_BITS));
        std::string denominator = std::to_string(1ll << FRAC_BITS);
        return numerator + "/" + denominator;
    }

    /*
     * Assigment operators of FixedPoint numbers.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint<INT_BITS, FRAC_BITS> &operator=(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS> &rhs) noexcept
    {
        this->num = rhs.num;
        this->round();
        return *this;
    }
    FixedPoint<INT_BITS, FRAC_BITS> &operator=(const FixedPoint<INT_BITS, FRAC_BITS> &rhs) noexcept
    {
        this->num = rhs.num;
        return *this;
    }
    FixedPoint<INT_BITS, FRAC_BITS> &operator=(int rhs) noexcept
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
        if ( num & (1ll << (31+INT_BITS)) )
        {
            long long res = this->get_num_sign_extended();
            return static_cast<double>(res) / static_cast<double>(1ll << 32);
        }
        else
        {
            return static_cast<double>(num) / 
                   static_cast<double>(1ll << 32);
        }
    }

    /*
     * Unary negation operator.
     */
    FixedPoint<INT_BITS, FRAC_BITS> operator-() const noexcept
    {
        FixedPoint<INT_BITS, FRAC_BITS> fix_res{};
        fix_res.num = -( this->get_num_sign_extended() );
        fix_res.round();
        return fix_res;
    }

    /*
     * Addition/subtraction of FixedPoint numbers. Result will have word length equal to
     * that of the left hand side of the operator.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint<INT_BITS, FRAC_BITS>
        operator+(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS> &rhs) const noexcept
    {
        FixedPoint<INT_BITS, FRAC_BITS> res;
        res.num = this->num + rhs.num;
        res.round();
        return res;
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint<INT_BITS, FRAC_BITS> &
        operator+=(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS> &rhs) noexcept
    {
        this->num += rhs.num;
        this->round();
        return *this;
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint<INT_BITS, FRAC_BITS>
        operator-(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS> &rhs) const noexcept
    {
        FixedPoint<INT_BITS, FRAC_BITS> res;
        res.num = this->num - rhs.num;
        res.round();
        return res;
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint<INT_BITS, FRAC_BITS> &
        operator-=(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS> &rhs) noexcept
    {
        this->num -= rhs.num;
        this->round();
        return *this;
    }

    /*
     * Multilication of FixedPoint numbers.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint<INT_BITS, FRAC_BITS>
        operator*(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS> &rhs) const noexcept
    {
        /*
         * Scenario 1:
         * The entire result of the multiplication can fit into one 64-bit
         * integer. This code produces significantly faster result when 
         * applicable.
         */
        if (INT_BITS+FRAC_BITS <= 32 && RHS_INT_BITS+RHS_FRAC_BITS <= 32)
        {
            FixedPoint<INT_BITS, FRAC_BITS> res{};
            long long op_a{ this->get_num_sign_extended() >> INT_BITS     };
            long long op_b{   rhs.get_num_sign_extended() >> RHS_INT_BITS };
            long long product{ op_a * op_b };

            // Shift result to the correct place.
            if (INT_BITS + RHS_INT_BITS > 32)
            {
                product <<= INT_BITS + RHS_INT_BITS - 32;
            }
            else
            {
                product >>= 32 - INT_BITS - RHS_INT_BITS;
            }
            res.num = product;
            res.round();
            return res;
        }
        /*
         * Scenario 2:
         * The entire result of the multiplication can fit into one 128-bit
         * integer. Running this code takes SIGNIFICANTLY longer time than
         * running the code of scenario 1, probably due to the fact that this
         * code won't be accelerated by any integer vectorization. However, this
         * piece of code seems to work for all sizes of FixedPoints.
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
            res.round();
            return res; 
        }
    }
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint<INT_BITS, FRAC_BITS> &
        operator*=(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS> &rhs) noexcept
    {
        FixedPoint<INT_BITS, FRAC_BITS> res{ *this * rhs };
        this->num = res.num;
        return *this;
    }

    /*
     * Division of FixedPoint numbers with integers. Result will have word
     * length equal to that of the left hand side of the operator, but the
     * precision of the result will not necessary represent such a wide number.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint<INT_BITS, FRAC_BITS>
        operator/(const FixedPoint<RHS_INT_BITS, RHS_FRAC_BITS> &rhs) const
    {
        // Note that Q(a,64) / Q(b,32) == Q(c,32).
        FixedPoint<INT_BITS, FRAC_BITS> res{};
        __extension__ __int128 dividend{ this->get_num_sign_extended() };
        __extension__ __int128 divisor {   rhs.get_num_sign_extended() };
        dividend <<= 32;

        // Create and return result. Note that Q(a,64) / Q(b,32) == Q(c,32).
        res.num = static_cast<long long>( dividend/divisor );
        res.round();
        return res;
    }
    FixedPoint<INT_BITS, FRAC_BITS>
        operator/(int rhs) const
    {
        FixedPoint<INT_BITS, FRAC_BITS> res;
        res.num = this->num / rhs;
        res.round();
        return res;
    }
    FixedPoint<INT_BITS, FRAC_BITS> &
        operator/=(int rhs)
    {
        this->num /= rhs;
        this->round();
        return *this;
    }
};

/*
 * Print-out to C++ stream object on the form '<int> + <frac>/<2^<frac_bits>'.
 * Good for debuging'n'stuff.
 */
template <int INT_BITS, int FRAC_BITS>
std::ostream &operator<<(std::ostream &os, const FixedPoint<INT_BITS, FRAC_BITS> &rhs)
{
    // Test if sign bit is set.
    if ( (1ll << (31+INT_BITS)) & rhs.num )
    {
        // Sign extend such that the number is interpreted correctly by C++.
        int integer = rhs.get_int() | ~((1 << INT_BITS) - 1);
        return os << integer << " + " << rhs.get_frac_quotient();
    }
    else
    {
        // No need to sign extend. Just grab the integer part.
        return os << rhs.get_int() << " + " << rhs.get_frac_quotient();
    }
}
