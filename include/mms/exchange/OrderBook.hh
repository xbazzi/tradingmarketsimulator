// C++ Includes
#include <cstddef>
#include <flat_set>

// Third Party Includes
#include <fiah/structs/SPSCQueue.hh>

// MarketMakerSimulator Includes
#include "mms/structs/Structs.hh"
#include "mms/utils/Types.hh"

namespace mms {
template <std::size_t N>
concept size_constraint = N < (1ULL << 32);

template <std::size_t SIZE, StringT SYMBOL>
    requires size_constraint<SIZE>
class OrderBook
{
public:

    struct Order
    {
        char symbol[8];
        enum class Side : std::uint8_t
        {
            Bid,
            Ask
        } side;
        Price price;
        std::uint32_t qty;
        std::uint32_t id;
        std::uint64_t rcv_ts;
        std::uint8_t flags;
    };

    // class PriceLevel;
    OrderBook() noexcept;
    ~OrderBook();

    bool add(Order order) noexcept;
    bool remove(std::uint32_t id) noexcept;

    class PriceLevel
    {
        bool try_match();
    };

    static void f();


protected:

private:
    std::vector<Order> m_bids;
    std::vector<Order> m_asks;
    std::flat_set<std::uint32_t> m_order_ids;


};

template <std::size_t SIZE, StringT SYMBOL>
    requires size_constraint<SIZE>
OrderBook<SIZE, SYMBOL>::OrderBook() noexcept
{

}

template <std::size_t SIZE, StringT SYMBOL>
    requires size_constraint<SIZE>
bool OrderBook<SIZE, SYMBOL>::add(Order order) noexcept
{

    const auto id = order.id;
    const auto side = order.side;
    if (m_order_ids.contains(order.id))
    {
        return false;
    }

    return bInserted = side == Order::Side::Bid? m_bids.push(Order): m_bids.push(Order);
}

template <std::size_t SIZE, StringT SYMBOL>
    requires size_constraint<SIZE>
bool OrderBook<SIZE, SYMBOL>::remove(std::uint32_t id) noexcept
{
    const auto id = order.id;
    const auto side = order.side;
    if (m_order_ids.contains(order.id))
    {
        return false;
    }

    return bInserted = side == Order::Side::Bid? m_bids.push(Order): m_bids.push(Order);
}

void f(int*& a)
{
    *a = 3;
}

int calculate()
{
    int x = 6;
    int* ptr = &x;
    int*& ref = ptr;
    // int& y = *ptr;
    f(ref);

}

} // End namespace mms
