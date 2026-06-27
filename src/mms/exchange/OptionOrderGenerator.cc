#include <algorithm>
#include <ranges>
#include <random>
#include <cstring>

#include "mms/exchange/OptionOrderGenerator.hh"

namespace mms {

OptionOrderGenerator::OptionOrderGenerator(AddFn add_fn, RemoveFn remove_fn,
                                           Option option, OptionOrderGeneratorConfig config)
    : m_add_fn{std::move(add_fn)},
      m_remove_fn{std::move(remove_fn)},
      m_option{option},
      m_config{config}
{}

Order OptionOrderGenerator::_make_order(Order::Side side, Price price, std::uint32_t qty) noexcept
{
    Order order{};
    std::memcpy(order.symbol.data, m_option.symbol.data, Symbol::MAX_SYMBOL_SIZE);
    order.side   = side;
    order.price  = price;
    order.qty    = qty;
    order.id     = ++m_next_id;
    order.rcv_ts = fiah::TimeStamp{}.update_now().get_ticks();
    order.flags  = m_option.flags;
    return order;
}

void OptionOrderGenerator::_track(const Order& order) noexcept
{
    m_live_orders.push_back(order);
}

void OptionOrderGenerator::_untrack(std::uint32_t id) noexcept
{
    auto it = std::ranges::find(m_live_orders, id, &Order::id);
    if (it != m_live_orders.end())
        m_live_orders.erase(it);
}

Price OptionOrderGenerator::_best_bid() const noexcept
{
    const Price fallback{static_cast<Price::type>(m_config.mid_price)
                         - static_cast<Price::type>(m_config.spread_half_width)};
    auto bids = m_live_orders
        | std::views::filter([](const Order& o){ return o.side == Order::Side::Bid; });
    if (std::ranges::empty(bids))
        return fallback;
    return std::ranges::max_element(bids, std::less{}, &Order::price)->price;
}

Price OptionOrderGenerator::_best_ask() const noexcept
{
    const Price fallback{static_cast<Price::type>(m_config.mid_price)
                         + static_cast<Price::type>(m_config.spread_half_width)};
    auto asks = m_live_orders
        | std::views::filter([](const Order& o){ return o.side == Order::Side::Ask; });
    if (std::ranges::empty(asks))
        return fallback;
    return std::ranges::min_element(asks, std::less{}, &Order::price)->price;
}

Order OptionOrderGenerator::generate_mm_order(Order::Side side) noexcept
{
    std::uniform_int_distribution<Price::type> spread_dist{
        1, static_cast<Price::type>(m_config.spread_half_width)};
    std::uniform_int_distribution<std::uint32_t> qty_dist{1u, m_config.mm_lot_max};

    const auto mid    = static_cast<Price::type>(m_config.mid_price);
    const auto jitter = spread_dist(m_prng);
    const Price price = (side == Order::Side::Bid)? Price{mid - jitter}:
                                                    Price{mid + jitter};

    return _make_order(side, price, qty_dist(m_prng));
}

Order OptionOrderGenerator::generate_aggressor_order(Order::Side side) noexcept
{
    std::uniform_int_distribution<std::uint32_t> qty_dist{1u, m_config.aggressor_lot_max};
    const auto offset = static_cast<Price::type>(m_config.aggressor_offset);

    const Price price = (side == Order::Side::Bid)?
        Price{static_cast<Price::type>(_best_ask()) + offset}:
        Price{static_cast<Price::type>(_best_bid()) - offset};

    return _make_order(side, price, qty_dist(m_prng));
}

std::optional<std::uint32_t> OptionOrderGenerator::pick_cancel_id() noexcept
{
    if (m_live_orders.empty())
        return std::nullopt;
    std::uniform_int_distribution<std::size_t> idx_dist{0uz, m_live_orders.size() - 1};
    return m_live_orders[idx_dist(m_prng)].id;
}

void OptionOrderGenerator::seed(Order order) noexcept
{
    _track(order);
}

void OptionOrderGenerator::notify_fill(std::uint32_t id) noexcept
{
    _untrack(id);
}

void OptionOrderGenerator::start() noexcept
{
    m_thread = std::jthread{[this](std::stop_token st){ run(st); }};
}

void OptionOrderGenerator::run(std::stop_token stop) noexcept
{
    std::uniform_real_distribution<float> action_dist{0.0f, 1.0f};
    std::uniform_int_distribution<int> side_dist{0, 1};

    while (!stop.stop_requested())
    {
        const float r = action_dist(m_prng);

        if (r < m_config.cancel_weight)
        {
            if (auto id = pick_cancel_id())
            {
                m_remove_fn(*id);
                _untrack(*id);
            }
        }
        else if (r < m_config.cancel_weight + m_config.aggressor_weight)
        {
            const auto side = side_dist(m_prng) == 0? Order::Side::Bid: Order::Side::Ask;
            const auto order = generate_aggressor_order(side);
            if (m_add_fn(order))
                _track(order);
        }
        else
        {
            const auto bid = generate_mm_order(Order::Side::Bid);
            const auto ask = generate_mm_order(Order::Side::Ask);
            if (m_add_fn(bid)) _track(bid);
            if (m_add_fn(ask)) _track(ask);
        }

        std::this_thread::sleep_for(m_config.sleep_dur);
    }
}

} // namespace mms
