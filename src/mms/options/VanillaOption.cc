#include "mms/options/VanillaOption.hh"
#include <cmath>
#include <memory>
#include <print>

namespace mms::options
{

// Cumulative distribution function for Gaussian distribution
double N(const double x)
{
    double k = 1.0 / (1.0 + 0.2316419 * x);
    double k_sum = k * (0.319381530 + k * (-0.356563782 + k * (1.781477937 + k * (-1.821255978 + 1.330274429 * k))));

    if (x >= 0.0)
    {
        return (1.0 - (1.0 / (pow(2 * M_PI, 0.5))) * exp(-0.5 * x * x) * k_sum);
    }
    else
    {
        return 1.0 - N(-x);
    }
}

// Parameterized constructor
VanillaOption::VanillaOption(const double &K, const double &r, const double &T, const double &S, const double sigma)
    : _K{K}, _r{r}, _T{T}, _S{S}, _sigma{sigma}
{
}

// Default constructor
VanillaOption::VanillaOption()
{
    init();
}

VanillaOption::VanillaOption(const VanillaOption &other)
{
    copy(other);
}

VanillaOption &VanillaOption::operator=(const VanillaOption &rhs)
{
    if (this == &rhs)
        return *this;
    copy(rhs);
    return *this;
}

void VanillaOption::copy(const VanillaOption &rhs)
{
    _K = rhs._K;
    _r = rhs._r;
    _T = rhs._T;
    _S = rhs._S;
    _sigma = rhs._sigma;
}

/// No point in a move constructor for primitive members
// VanillaOption::VanillaOption(VanillaOption&& rhs)
// {
//     _K = rhs._K;
//     _r = rhs._r;
//     _T = rhs._T;
//     _S = rhs._S;
//     _sigma = rhs._sigma;
// }

VanillaOption::~VanillaOption()
{
}

int VanillaOption::init()
{
    _K = 100.0; // Strike price
    _r = 0.05;
    _T = 0.1;   // Time until maturity (years)
    _S = 100.0; // Option is at the money (spot = strike)
    _sigma = 0.2;

    return 1;
}

double VanillaOption::get_K() const
{
    return _K;
}

double VanillaOption::get_T() const
{
    return _T;
}

double VanillaOption::get_S() const
{
    return _S;
}

double VanillaOption::get_r() const
{
    return _r;
}

double VanillaOption::get_sigma() const
{
    return _sigma;
}

double VanillaOption::calc_call_price() const
{
    double sigma_sqrt_T = _sigma * sqrt(_T);
    double d_1 = (log(_S / _K) + (_r + _sigma * _sigma * 0.5) * _T) / sigma_sqrt_T;
    double d_2 = d_1 - sigma_sqrt_T;
    return _S * N(d_1) - _K * exp(-_r * _T) * N(d_2);
}

double VanillaOption::calc_put_price() const
{
    double sigma_sqrt_T = _sigma * sqrt(_T);
    double d_1 = (log(_S / _K) + (_r + _sigma * _sigma * 0.5) * _T) / sigma_sqrt_T;
    double d_2 = d_1 - sigma_sqrt_T;
    return _K * exp(-_r * _T) * N(-d_2) - _S * N(-d_1);
}
} // namespace mms::options