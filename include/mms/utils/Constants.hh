#pragma once

// C++ Includes
#include <cstdint>

// MarketMakerSimulator Includes

namespace mms::utils
{
/**
 * @brief Constants used throughout the MarketMakerSimulator project.
 */
#if defined(__cpp_lib_hardware_interference_size)
{
    static constexpr const std::uint16_t CACHE_LINE_SIZE =
        static_cast<std::uint16_t>(std::hardware_destructive_interference_size);
}
#else
{
    static constexpr const std::uint16_t CACHE_LINE_SIZE = 64;
}
#endif // __cpp_lib_hardware_interference_size
} // namespace mms::utils