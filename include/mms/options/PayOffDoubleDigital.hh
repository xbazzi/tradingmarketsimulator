#include "PayOff.hh"

namespace mms::options
{

class PayOffDoubleDigital : public PayOff
{
  private:
    double _U; // Upper strike price
    double _D; // Lower strike price

  public:
    // Two strike parameters for constructor
    PayOffDoubleDigital(const double _U, const double _D);

    // Destructor
    virtual ~PayOffDoubleDigital();

    // Pay-off is 1 if spot within strike barriers, 0 otherwise
    virtual double operator()(const double S) const;
};
} // namespace mms::options