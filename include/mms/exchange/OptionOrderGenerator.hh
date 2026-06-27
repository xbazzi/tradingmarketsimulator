#pragma once

#include <cstdint>
#include <vector>
#include <optional>
#include <functional>
#include <chrono>
#include <thread>
#include <stop_token>
#include <random>

#include "mms/structs/Structs.hh"
#include <fiah/utils/XorBitant.hh>
#include <fiah/utils/TimeStamp.hh>

namespace mms {

struct OptionOrderGeneratorConfig {
    Price mid_price;
    Price spread_half_width; 
    Price aggressor_offset;   
    std::uint32_t mm_lot_max;  
    std::uint32_t aggressor_lot_max;
    float aggressor_weight;     
    float cancel_weight;        
    std::chrono::nanoseconds sleep_dur;
};

class OptionOrderGenerator {
public:
    using AddFn    = std::function<bool(Order)>;
    using RemoveFn = std::function<bool(std::uint32_t)>;

    OptionOrderGenerator() = default;
    OptionOrderGenerator(AddFn add_fn, RemoveFn remove_fn,
                         Option option, OptionOrderGeneratorConfig config);

    void start() noexcept;

    Order generate_mm_order(Order::Side side) noexcept;
    Order generate_aggressor_order(Order::Side side) noexcept;
    std::optional<std::uint32_t> pick_cancel_id() noexcept;

    void seed(Order order) noexcept;
    void notify_fill(std::uint32_t id) noexcept;

private:
    void run(std::stop_token stop) noexcept;
    Order _make_order(Order::Side side, Price price, std::uint32_t qty) noexcept;
    void _track(const Order& order) noexcept;
    void _untrack(std::uint32_t id) noexcept;
    Price _best_bid() const noexcept;
    Price _best_ask() const noexcept;

    AddFn m_add_fn;
    RemoveFn m_remove_fn;
    Option m_option{};
    OptionOrderGeneratorConfig m_config{};
    std::vector<Order> m_live_orders;
    std::uint32_t m_next_id{};
    // mutable fiah::XorBitant m_prng{std::random_device{}()};
    mutable std::mt19937 m_prng{std::random_device{}()};
    std::jthread m_thread;
};

} // namespace mms
