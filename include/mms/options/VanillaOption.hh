#pragma once

namespace mms
{

class VanillaOption
{
  private:
    double _K;     // Strike price
    double _r;     // Risk-free rate
    double _T;     // Maturity time
    double _S;     // Underlying asset price
    double _sigma; // Volatility of underlying asset

  public:
    explicit VanillaOption(const double &K, const double &r, const double &T, const double &S, const double sigma);
    VanillaOption();

    VanillaOption(const VanillaOption &rhs);
    VanillaOption(VanillaOption &&rhs) = default;
    VanillaOption &operator=(const VanillaOption &rhs);
    virtual ~VanillaOption();

    int init();
    void copy(const VanillaOption &);
    double get_K() const;
    double get_r() const;
    double get_T() const;
    double get_S() const;
    double get_sigma() const;

    double calc_call_price() const;
    double calc_put_price() const;
};
} // namespace mms