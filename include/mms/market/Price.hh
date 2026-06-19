#include <cstdint>
#include <cstddef>
#include <string>
#include <limits>

namespace mms {
constexpr std::int32_t INVALID_PRICE{std::numeric_limits<int>::min()};

class Price32
{
public:

    Price32() : m_price{INVALID_PRICE} {}
    explicit Price32(std::int32_t price) : m_price{price} {}

    double tof()
    {
        return static_cast<float>(m_price);
    }

private:
    std::int32_t m_price;
};

using Price = Price32;
} // namespace mms