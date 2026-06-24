#pragma once

#include <cstddef>
#include <vector>
#include <array>
#include <ranges>
#include <algorithm>

using Quantity = std::uint32_t;
using OrderId = std::uint64_t;
using OrderIds = std::vector<OrderId>;

template <std::size_t SIZE>
    requires (SIZE <= 6)
struct StringT
{
    std::array<char, SIZE> data;
    consteval StringT(const char (&init)[SIZE]) 
    {
        std::ranges::copy(init, data.begin());
    }
};

