#include "mms/options/PayOffDoubleDigital.hh"

namespace mms
{

// Constructor with two strike parameters, upper and lower barrier
PayOffDoubleDigital::PayOffDoubleDigital(const double U, const double D) : _U{U}, _D{D}
{
}

// Destructor
PayOffDoubleDigital::~PayOffDoubleDigital()
{
}

// Over-ridden operator() method, which turns
// PayOffDoubleDigital into a function object
double PayOffDoubleDigital::operator()(const double S) const
{
    if (S >= _D && S <= _U)
    {
        return 1.0;
    }
    else
    {
        return 0.0;
    }
}
} // namespace mms