#include <iostream>
#include <string>

template <int INT_BITS, int FRAC_BITS>
class FixedPoint
{
    static_assert(INT_BITS <= 32, "Integer bits should be less than or equal to 32 bits.");
    static_assert(FRAC_BITS <= 32, "Fractional bits should be less than or equal to 32 bits.");
    const long long SHIFT_MASK = (1 << INT_BITS+32)-1 & ((1 << FRAC_BITS)-1 << 32-FRAC_BITS);

    /*
     * Long long is guaranteed to be atleast 64-bits wide. We use the 32 most 
     * significant bits to store the integer part and the 32 least significant
     * bits to store the fraction.
     */
    long long num;

    /*
     * Friend declaration for accessing 'num' between different types, i.e, 
     * between instances with different wordlenth.
     */
    template <int _INT_BITS, int _FRAC_BITS>
    friend class FixedPoint;

public:
    FixedPoint( ) { }
    FixedPoint(double a) { }

    /*
     * Basic getters and setters.
     */
    long get_int() const { return static_cast<long>((num >> 32) & 0xFFFFFFFF); }
    long get_frac() const { return static_cast<long>(num & 0xFFFFFFFF); }
    std::string get_frac_quotient() const 
    { 
        std::string numerator = std::to_string((num & 0xFFFFFFFF) >> 32-FRAC_BITS);
        std::string denominator = std::to_string(1 << FRAC_BITS);
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
    FixedPoint<INT_BITS, FRAC_BITS> operator+(const FixedPoint<RHS_INT_BITS,RHS_FRAC_BITS> &rhs)
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
    FixedPoint<INT_BITS, FRAC_BITS> operator/(int rhs)
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
    return os << rhs.get_int() << " + " << rhs.get_frac_quotient();
}

int main()
{
    FixedPoint<5,5> n;
    FixedPoint<4,4> m;
    FixedPoint<6,6> o;
    n = 5;
    m = 2;
    o = n + m;
    std::cout << o << std::endl;
    std::cout << o/2 << std::endl;
    return 0;
}
