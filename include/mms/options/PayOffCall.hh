#pragma once

#include "PayOff.hh"

namespace mms::options
{

class PayOffCall : public PayOff
{
  private:
    double _K;

  public:
    PayOffCall(const double K);
    virtual ~PayOffCall() {};
    virtual double operator()(const double S) const; // max(S - K, 0)
};
} // namespace mms::options