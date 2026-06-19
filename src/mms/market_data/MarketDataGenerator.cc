// clang-format on

// MarketMakerSimulator Includes
#include "mms/market_data/MarketDataGenerator.hh"
#include "mms/structs/Structs.hh"
// clang-format off

namespace mms 
{
DepthMessage MarketDataGenerator::generate_depth_message(Option option)
{
    return {Price{}, "AAPL", 26u, 7u, 4u};
}

void MarketDataGenerator::send_depth_message()
{
    return;
}

} // End namespace mms