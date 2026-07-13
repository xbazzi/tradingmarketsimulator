#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <limits>

namespace mms {

/// @todo switch to fiah types
class Price32
{
public:
    using type = std::int32_t;
    static constexpr type INVALID_PRICE{std::numeric_limits<int>::min()};

    Price32() : m_price{INVALID_PRICE} {}
    explicit Price32(type price) : m_price{price} {}

    bool is_valid() { return m_price != INVALID_PRICE; };

    template <bool IS_BID>
    static bool is_more_aggro(Price32 lhs, Price32 rhs)
    {
        if constexpr (IS_BID)
            return lhs.m_price > rhs.m_price;
        else
            return lhs.m_price < rhs.m_price;
    }

    static bool is_more_aggro(Price32 lhs, Price32 rhs, bool is_bid)
    {
        if (is_bid)
            return lhs.m_price > rhs.m_price;
        else
            return lhs.m_price < rhs.m_price;
    }

    template <bool IS_BID>
    static bool is_more_cons(Price32 lhs, Price32 rhs)
    {
        if constexpr (IS_BID)
            return lhs.m_price < rhs.m_price;
        else
            return lhs.m_price > rhs.m_price;
    }

    static bool is_more_cons(Price32 lhs, Price32 rhs, bool is_bid)
    {
        if (is_bid)
            return lhs.m_price < rhs.m_price;
        else
            return lhs.m_price > rhs.m_price;
    }

    double tof() const { return static_cast<float>(m_price) / 100.0f; }
    operator type() const noexcept { return m_price; }
    std::strong_ordering operator<=>(const Price32&) const noexcept = default;

private:
    type m_price;
};

using Price = Price32;
} // namespace mms