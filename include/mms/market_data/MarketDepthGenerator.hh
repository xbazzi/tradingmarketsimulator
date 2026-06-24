#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <algorithm>
#include <ranges>
#include <random>
#include <print>

#include "mms/error/Error.hh"
#include "mms/structs/Structs.hh"

#include <fiah/io/Udp.hh>
#include <fiah/structs/Vector.hh>
#include <fiah/utils/TimeStamp.hh>
#include <fiah/utils/XorBitant.hh>

namespace mms {
class MarketDepthGenerator
{
public:
    using MarketDepthPacket = std::array<std::byte, WireDepthMsgV1::size>;

    template <class Rep, class Period>
    explicit MarketDepthGenerator(fiah::UdpServer udp, std::span<fiah::UdpClient> clients, std::chrono::duration<Rep, Period>);

    MarketDepthGenerator() = default;

    ~MarketDepthGenerator() = default;

    void start() noexcept;

    template <bool PrintLog>
    void run() noexcept;

    void enqueue(const MarketDepthMessage& msg) noexcept;

    void send_msg() noexcept;

    MarketDepthMessage generate_depth_message() noexcept;

    template <std::ranges::input_range T>
        requires std::convertible_to<Symbol, std::ranges::range_value_t<T>>
    void fill_symbols(const T& init);

    template <std::ranges::input_range T>
        requires std::convertible_to<Price, std::ranges::range_value_t<T>>
    void fill_strikes(const T& init);

    template <std::ranges::input_range T>
        requires std::convertible_to<Price, std::ranges::range_value_t<T>>
    void fill_baseprice(const T& init);

private:
    Option _generate_option(std::size_t idx) const noexcept;
    MarketDepthMessage::Header _generate_header() noexcept;
    MarketDepthPacket _serialize(const MarketDepthMessage& msg) noexcept;
    void _write_u8(MarketDepthPacket& buf, std::size_t offset, std::uint8_t val) noexcept;
    void _write_u16(MarketDepthPacket& buf, std::size_t offset, std::uint16_t val) noexcept;
    void _write_u32(MarketDepthPacket& buf, std::size_t offset, std::uint32_t val) noexcept;
    void _write_u64(MarketDepthPacket& buf, std::size_t offset, std::uint64_t val) noexcept;

private:
    fiah::SPSCQueue<MarketDepthMessage, 1 << 10> m_send_q;

    std::vector<fiah::UdpClient> m_clients;
    std::vector<Symbol> m_symbols;
    std::vector<Price> m_strikes;
    std::vector<Price> m_base_prices;

    fiah::UdpServer m_udp;
    std::jthread m_run_thread;
    mutable fiah::XorBitant m_prng;
    std::uint16_t m_seq;
    std::chrono::nanoseconds m_sleep_dur;
};

template <class Rep, class Period>
MarketDepthGenerator::MarketDepthGenerator(fiah::UdpServer udp, std::span<fiah::UdpClient> clients, std::chrono::duration<Rep, Period> sleep_dur) 
    : m_udp{std::move(udp)},
      m_prng{std::random_device{}()},
      m_seq{},
      m_sleep_dur{sleep_dur}
{
    m_udp.start();

    std::move(clients.begin(), clients.end(), std::back_inserter(m_clients));
    fill_symbols(std::initializer_list{Symbol{"AAPL"}, Symbol{"AMZN"}, Symbol{"SPCX"}, Symbol{"TSLA"}, Symbol{"MSFT"}});
    fill_strikes(std::initializer_list{Price{20000}, Price{30000}, Price{20000}, Price{80000}, Price{100000}});
    fill_baseprice(std::initializer_list{Price{21000}, Price{32000}, Price{16000}, Price{85000}, Price{92000}});
}

template <bool print_log>
void MarketDepthGenerator::run() noexcept
{
    while (true)
    {
        const auto msg = generate_depth_message();
        enqueue(msg);
        if constexpr (print_log)
            std::print("[{}]Sent: {}{:2d}{:02d}{:02d}{}{:0.2f}, seq={}\n",
                msg.header.ts_ns,
                msg.option.symbol.data, msg.option.year, msg.option.month, msg.option.day,
                (msg.option.flags bitand std::to_underlying(mms::Flags::IsCall))? "C": "P", 
                msg.option.strike.tof(), msg.header.seq
            );
        send_msg();
        std::this_thread::sleep_for(m_sleep_dur);
    }
}

template <std::ranges::input_range T>
    requires std::convertible_to<Symbol, std::ranges::range_value_t<T>>
void MarketDepthGenerator::fill_symbols(const T& init)
{
    std::copy(init.begin(), init.end(), std::back_inserter(m_symbols));
}

template <std::ranges::input_range T>
    requires std::convertible_to<Price, std::ranges::range_value_t<T>>
void MarketDepthGenerator::fill_strikes(const T& init)
{
    std::copy(init.begin(), init.end(), std::back_inserter(m_strikes));
}

template <std::ranges::input_range T>
    requires std::convertible_to<Price, std::ranges::range_value_t<T>>
void MarketDepthGenerator::fill_baseprice(const T& init)
{
    std::copy(init.begin(), init.end(), std::back_inserter(m_base_prices));
}
} // End namespace mms
