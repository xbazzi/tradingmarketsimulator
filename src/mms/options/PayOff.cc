#include "mms/options/PayOffCall.hh"
#include "mms/options/PayOffPut.hh"

namespace mms::options
{

PayOff::PayOff()
{
}

// =========
// PayOffCall
// =========

// Constructor with single strike parameter
PayOffCall::PayOffCall(const double K) : _K{K}
{
}

// Over-ridden operator() method, which turns PayOffCall into a function object
double PayOffCall::operator()(const double S) const
{
    return std::max(S - _K, 0.0); // Standard European call pay-off
}

// =========
// PayOffPut
// =========

// Constructor with single strike parameter
PayOffPut::PayOffPut(const double K) : _K{K}
{
}

// Over-ridden operator() method, which turns PayOffPut into a function object
double PayOffPut::operator()(const double S) const
{
    return std::max(_K - S, 0.0); // Standard European put pay-off
}

} // namespace mms::options