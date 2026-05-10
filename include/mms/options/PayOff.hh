#pragma once

#include <algorithm>

namespace mms::options
{

class PayOff
{
  public:
    PayOff();
    virtual ~PayOff() {};

    virtual double operator()(const double S) const = 0;
};
} // namespace mms::options