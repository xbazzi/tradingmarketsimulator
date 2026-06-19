#include <cstdint>

#include "mms/structs/Structs.hh"


// MarketMakerSimulator Includes

namespace mms {
class MarketDataGenerator
{
public:
    MarketDataGenerator();
    ~MarketDataGenerator();

    void send_depth_message();
    DepthMessage generate_depth_message(Option);

protected:

private:

};
} // End namespace mm