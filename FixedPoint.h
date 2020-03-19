#include <iosfwd>
#include <string>

template <int INT_BITS, int FRAC_BITS>
class FixedPoint
{
    static_assert(INT_BITS <= 32, "Integer bits should be less than or equal to 32 bits.");
    static_assert(FRAC_BITS <= 32, "Fractional bits should be less than or equal to 32 bits.");
    const long long SHIFT_MASK = (1ll << (INT_BITS+32))-1 | (((1ll << FRAC_BITS)-1) << (32-FRAC_BITS));

    /*
     * Long long is guaranteed to be atleast 64-bits wide. We use the 32 most 
     * significant bits to store the integer part and the 32 least significant
     * bits to store the fraction.
     */
    long long num{};

    /*
     * Friend declaration for accessing 'num' between different types, i.e, 
     * between instances with different wordlenth.
     */
    template <int _INT_BITS, int _FRAC_BITS>
    friend class FixedPoint;

    /*
     * Friend declaration for stream output method.
     */
    template <int _INT_BITS, int _FRAC_BITS>
    friend std::ostream &operator<<(std::ostream &os, const FixedPoint<_INT_BITS, _FRAC_BITS> &rhs);

public:
    FixedPoint() = default;

    /*
     * Constructor for floating point number input.
     */
    FixedPoint(double a)
    { 
        // Shifting a negative signed value is implementation defined: (ISO/IEC 
        // 9899:1999 Section 6.5.7). For GNU g++ (9.2.0) and CLANG++ (8.0.1) 
        // this solution seems to work.
        double fraction = (a - static_cast<long long>(a)) * (1ll << FRAC_BITS);
        num  = static_cast<long long>(a) << 32;
        num |= static_cast<long long>(std::abs(fraction)) << (32-FRAC_BITS);
        num &= SHIFT_MASK;
    }

    /*
     * Basic getters and setters.
     */
    int get_int_bits() const noexcept { return INT_BITS; }
    int get_frac_bits() const noexcept { return FRAC_BITS; }
    int get_int() const noexcept { return static_cast<int>(num >> 32); }
    int get_frac() const noexcept { return static_cast<int>(num & 0xFFFFFFFF); }
    std::string get_frac_quotient() const noexcept
    { 
        std::string numerator = std::to_string((num & 0xFFFFFFFF) >> (32-FRAC_BITS));
        std::string denominator = std::to_string(1ll << FRAC_BITS);
        return numerator + "/" + denominator;
    }

    /*
     * Assigment of FixedPoint numbers.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint<INT_BITS, FRAC_BITS> &operator=(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS> &rhs)
    {
        this->num = rhs.num;
        num &= SHIFT_MASK;
        return *this;
    }
    FixedPoint<INT_BITS, FRAC_BITS> &operator=(int rhs)
    {
        this->num = static_cast<long long>(rhs) << 32;
    }

    /*
     * Addition of FixedPoint numbers. Result will have word length equal to
     * that of the left hand side of the operator.
     */
    template <int RHS_INT_BITS, int RHS_FRAC_BITS>
    FixedPoint<INT_BITS, FRAC_BITS> 
        operator+(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS> &rhs) const
    {
        FixedPoint<INT_BITS, FRAC_BITS> res;
        res.num = this->num + rhs.num;
        res.num &= SHIFT_MASK;
        return res;
    }

    /*
     * Division of FixedPoint numbers. Result will have word length equal to
     * that of the left hand side of the operator.
     */
    FixedPoint<INT_BITS, FRAC_BITS> 
        operator/(int rhs) const
    {
        FixedPoint<INT_BITS, FRAC_BITS> res;
        res.num = this->num / rhs;
        res.num &= SHIFT_MASK;
        return res;
    }
};

template <int INT_BITS, int FRAC_BITS>
std::ostream &operator<<(std::ostream &os, const FixedPoint<INT_BITS, FRAC_BITS> &rhs)
{
    // Test if sign bit is set.
    if ( (1ll << (31+INT_BITS)) & rhs.num )
    {
        // Sign extend such that the number is interpreted correctly by C++.
        int integer = rhs.get_int() | ~((1 << INT_BITS) - 1);
        return os << integer << " - " << rhs.get_frac_quotient();
    }
    else
    {
        // No need to sign extend. Just grab the integer part.
        return os << rhs.get_int() << " + " << rhs.get_frac_quotient();
    }
}
