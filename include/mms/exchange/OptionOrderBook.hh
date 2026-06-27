// C++ Includes
#include <cstddef>
#include <functional>
#include <vector>
#include <robin_hood.h>
#include <ranges>
#include <format>

// Third Party Includes
#include <fiah/structs/SPSCQueue.hh>

// MarketMakerSimulator Includes
#include "mms/structs/Structs.hh"
#include "mms/utils/Types.hh"

namespace mms {
template <std::size_t N>
concept size_constraint = N < (1ULL << 32);

struct OptionOrderBookConfig
{
    //
};

/// @brief Price-time allocation order book
/// @tparam SIZE 
/// @tparam SYMBOL 
/// @tparam LOG 
template <std::size_t SIZE, StringT SYMBOL, bool LOG>
    requires size_constraint<SIZE>
class OptionOrderBook
{
public:

    using OrderT = Order;
    using ContainerT = std::vector<OrderT>;
    using Side = Order::Side;
    using MapT = robin_hood::unordered_flat_map<std::uint32_t, Side>;
    using FillFn = std::move_only_function<void(std::uint32_t)>;

    OptionOrderBook() noexcept;
    OptionOrderBook(FillFn&& fill_fn) noexcept;
    OptionOrderBook(const OptionOrderBookConfig& cfg) noexcept;
    ~OptionOrderBook();

    bool has(std::uint32_t id) const noexcept;
    auto& get_container(this auto&& self, Side side) noexcept;
    auto& get_map(this auto&& self) noexcept;
    bool add(Order order) noexcept;
    bool remove(std::uint32_t id) noexcept;
    std::uint32_t try_match(Order& order) noexcept;
    void set_fill_fn(FillFn fn) noexcept { m_fill_fn = std::move(fn); }

protected:

private:
    ContainerT m_bids;
    ContainerT m_asks;
    MapT m_order_ids_to_side;
    FillFn m_fill_fn;
};

template <std::size_t SIZE, StringT SYMBOL, bool LOG>
    requires size_constraint<SIZE>
OptionOrderBook<SIZE, SYMBOL, LOG>::OptionOrderBook() noexcept
{
    m_bids.reserve(SIZE);
    m_asks.reserve(SIZE);
}

template <std::size_t SIZE, StringT SYMBOL, bool LOG>
    requires size_constraint<SIZE>
OptionOrderBook<SIZE, SYMBOL, LOG>::OptionOrderBook(FillFn&& fill_fn) noexcept
    : m_fill_fn{std::move(fill_fn)}
{
    m_bids.reserve(SIZE);
    m_asks.reserve(SIZE);
}

template <std::size_t SIZE, StringT SYMBOL, bool LOG>
    requires size_constraint<SIZE>
OptionOrderBook<SIZE, SYMBOL, LOG>::~OptionOrderBook()
{
}

template <std::size_t SIZE, StringT SYMBOL, bool LOG>
    requires size_constraint<SIZE>
bool OptionOrderBook<SIZE, SYMBOL, LOG>::has(std::uint32_t id) const noexcept
{
    return m_order_ids_to_side.contains(id);
}

template <std::size_t SIZE, StringT SYMBOL, bool LOG>
    requires size_constraint<SIZE>
auto& OptionOrderBook<SIZE, SYMBOL, LOG>::get_container(this auto&& self, const Side side) noexcept
{
    return (side == Side::Bid)? self.m_bids: self.m_asks;
}

template <std::size_t SIZE, StringT SYMBOL, bool LOG>
    requires size_constraint<SIZE>
auto& OptionOrderBook<SIZE, SYMBOL, LOG>::get_map(this auto&& self) noexcept
{
    return self.m_order_ids_to_side;
}

/// @todo maybe add rvalue ref overload or do some template magic to deduce order type
/// @todo needs proper sorted insertion for bids and asks

/// @brief 
/// @tparam SIZE 
/// @tparam SYMBOL 
/// @tparam LOG 
/// @param order 
/// @return true if inserted into book
template <std::size_t SIZE, StringT SYMBOL, bool LOG>
    requires size_constraint<SIZE>
bool OptionOrderBook<SIZE, SYMBOL, LOG>::add(Order order) noexcept
{
    if (!order.is_valid())
        return false;

    if constexpr (LOG)
        std::print("Trying to add {}\n", order);

    const auto original_qty = order.qty;
    const auto consumed_qty = try_match(order);
    const bool fully_consumed = consumed_qty == original_qty;
    if (fully_consumed) // no need to insert into book
    {
        if constexpr (LOG) { std::print("Fully consumed {}\n", order);
                             std::print("Current state: {}\n", *this); }
        return false;
    }

    if (consumed_qty == 0)
        if constexpr (LOG) { std::print("No match {}\n", order); }


    const auto id = order.id;
    const auto side = order.side;
    const auto is_bid = order.is_bid();
    const auto& [map_it, inserted] = m_order_ids_to_side.try_emplace(id, side);
    if (!inserted)
    {
        if constexpr (LOG) { std::print("Failed to add {}\n", order);
                             std::print("Current state: {}\n", *this); }
        return false;
    }

    ContainerT& container = get_container(side);
    if (container.size() == SIZE)
    {
        m_order_ids_to_side.erase(id);
        if constexpr (LOG) { std::print("Container full, could not add {}\n", order);
                             std::print("Current state: {}\n", *this); }
        return false;
    }

    auto sort_fn = [&order, is_bid](const auto& p){ return is_bid? p <= order.price: p >= order.price; };
    auto it = std::ranges::find_if(container, sort_fn, &Order::price);
    container.insert(it, order);
    if constexpr (LOG) { std::print("Added {}\n", order);
                         std::print("Current state: {}\n", *this); }
    return true;
}

template <std::size_t SIZE, StringT SYMBOL, bool LOG>
    requires size_constraint<SIZE>
bool OptionOrderBook<SIZE, SYMBOL, LOG>::remove(std::uint32_t id) noexcept
{

    const auto map_it = m_order_ids_to_side.find(id);
    if (map_it == m_order_ids_to_side.end())
        return false;

    const auto side = map_it->second;
    ContainerT& container = get_container(side);
    auto it = std::ranges::find(container, id, &Order::id);
    container.erase(it);
    m_order_ids_to_side.erase(map_it);
    return true;
}

template <std::size_t SIZE, StringT SYMBOL, bool LOG>
    requires size_constraint<SIZE>
std::uint32_t OptionOrderBook<SIZE, SYMBOL, LOG>::try_match(Order& aggressor) noexcept
{
    const auto opp_side = aggressor.opp_side();
    const bool aggressor_is_bid = aggressor.is_bid();
    const auto orig_aggressor_qty = aggressor.qty;
    auto& container = get_container(opp_side);

    std::vector<std::uint32_t> ids_to_remove;
    ids_to_remove.reserve(container.size());

    for (Order& passive: container)
    {
        const bool tradeable_price = !Price::is_more_cons(aggressor.price, passive.price, aggressor_is_bid);
        const bool consumed_aggressor = passive.qty > aggressor.qty;
        const bool consumed_passive = passive.qty < aggressor.qty;

        if (tradeable_price)
        {
            if (consumed_passive)
            {
                ids_to_remove.push_back(passive.id);
                aggressor.qty -= passive.qty;
                if constexpr(LOG)
                    std::print("Consumed passive id={}, aggressor: {}\n", passive.id, aggressor);
            }
            else if (consumed_aggressor)
            {
                passive.qty -= aggressor.qty;
                aggressor.qty = 0;
                if constexpr(LOG)
                    std::print("Consumed aggressor, passive: {}\n", passive);
                break;
            }
            else // consumed both
            {
                ids_to_remove.push_back(passive.id);
                aggressor.qty = 0;
                passive.qty = 0;
                if constexpr(LOG)
                    std::print("Consumed both: aggressor: {}, passive: {}\n", aggressor, passive);
                break;
            }
        }
    }

    if (!ids_to_remove.empty())
        std::ranges::for_each(ids_to_remove, [this](const auto id){
            if constexpr (LOG)
                std::print("Removing passive id={}\n", id);
            remove(id);
            if (m_fill_fn)
                m_fill_fn(id);
        });

    return orig_aggressor_qty - aggressor.qty;
}

} // End namespace mms


template <std::size_t SIZE, StringT SYMBOL, bool LOG>
struct std::formatter<mms::OptionOrderBook<SIZE, SYMBOL, LOG>> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const mms::OptionOrderBook<SIZE, SYMBOL, LOG>& book, std::format_context& ctx) const
    {
        using Side = mms::Order::Side;
        auto out = ctx.out();
        const auto& bids = book.get_container(Side::Bid);
        const auto& asks = book.get_container(Side::Ask);
        const auto rows = std::max(bids.size(), asks.size());

        out = std::format_to(out, "OrderBook[{}]:  Bids({}) Asks({})\n",
                             std::string_view{SYMBOL.data.data()}, bids.size(), asks.size());
        out = std::format_to(out, "  {:>6}  {:>6} | {:<6}  {:<6}\n", "qty", "bid", "ask", "qty");
        out = std::format_to(out, "  {:->6}--{:->6}-+-{:-<6}--{:-<6}\n", "", "", "", "");

        for (std::size_t i = 0; i < rows; ++i)
        {
            if (i < bids.size())
                out = std::format_to(out, "  {:>6}  {:>6}", bids[i].qty,
                                     static_cast<mms::Price::type>(bids[i].price));
            else
                out = std::format_to(out, "  {:>6}  {:>6}", "", "");

            out = std::format_to(out, " | ");

            if (i < asks.size())
                out = std::format_to(out, "{:<6}  {:<6}\n",
                                     static_cast<mms::Price::type>(asks[i].price), asks[i].qty);
            else
                out = std::format_to(out, "{:<6}  {:<6}\n", "", "");
        }
        return out;
    }
};
