#include <cstdint>
#include <cstddef>
#include <string>
#include <limits>

namespace mms {
constexpr std::int32_t INVALID_PRICE{std::numeric_limits<int>::min()};

class Price32
{
public:
    using type = std::int32_t;

    Price32() : m_price{INVALID_PRICE} {}
    explicit Price32(type price) : m_price{price} {}

    double tof()
    {
        return static_cast<float>(m_price);
    }

    operator type() const noexcept
    {
        return m_price;
    }

private:
    type m_price;
};

using Price = Price32;
} // namespace mms