#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <algorithm>
#include <ranges>

#include "mms/error/Error.hh"
#include "mms/structs/Structs.hh"

#include <fiah/io/Udp.hh>
#include <fiah/structs/Vector.hh>
#include <fiah/utils/TimeStamp.hh>

namespace mms {
class MarketDataGenerator
{
public:
    static constexpr std::size_t DEPTH_MSG_LEN{sizeof(MarketDepthMessage)};
    MarketDataGenerator()
    {
        fill_symbols(std::initializer_list{Symbol{"AAPL"}, Symbol{"AMZN"}, Symbol{"SPCX"}, Symbol{"TSLA"}, Symbol{"MSFT"}});
        fill_strikes(std::initializer_list{Price{20000}, Price{30000}, Price{20000}, Price{80000}, Price{100000}});
    }
    explicit MarketDataGenerator(fiah::UdpServer udp, std::span<fiah::UdpClient> clients) 
        : m_udp{std::move(udp)}
    {
        std::move(clients.begin(), clients.end(), std::back_inserter(m_clients));
        fill_symbols(std::initializer_list{Symbol{"AAPL"}, Symbol{"AMZN"}, Symbol{"SPCX"}, Symbol{"TSLA"}, Symbol{"MSFT"}});
        fill_strikes(std::initializer_list{Price{20000}, Price{30000}, Price{20000}, Price{80000}, Price{100000}});
    }
    ~MarketDataGenerator() = default;

    std::expected<void, Error> setup() noexcept;

    void enqueue(const MarketDepthMessage& msg) noexcept;

    void send_depth_message() noexcept;

    MarketDepthMessage generate_depth_message(fiah::TimeStamp<> ts, Option option) const noexcept;

    Option generate_option() const;

    template <std::ranges::input_range T>
        requires std::convertible_to<Symbol, std::ranges::range_value_t<T>>
    void fill_symbols(const T& init);

    template <std::ranges::input_range T>
        requires std::convertible_to<Price, std::ranges::range_value_t<T>>
    void fill_strikes(const T& init);

private:
    fiah::SPSCQueue<MarketDepthMessage, 1 << 20> m_send_q;
    fiah::UdpServer m_udp;
    std::vector<fiah::UdpClient> m_clients;
    std::vector<Symbol> m_symbols;
    std::vector<Price> m_strikes;
};

template <std::ranges::input_range T>
    requires std::convertible_to<Symbol, std::ranges::range_value_t<T>>
void MarketDataGenerator::fill_symbols(const T& init)
{
    std::copy(init.begin(), init.end(), std::back_inserter(m_symbols));
}

template <std::ranges::input_range T>
    requires std::convertible_to<Price, std::ranges::range_value_t<T>>
void MarketDataGenerator::fill_strikes(const T& init)
{
    std::copy(init.begin(), init.end(), std::back_inserter(m_strikes));
}
} // End namespace mms
