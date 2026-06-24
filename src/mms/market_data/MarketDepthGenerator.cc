// MarketMakerSimulator Includes
#include <cstring>
#include <random>
#include <chrono>
#include <cstddef>

#include "mms/market_data/MarketDepthGenerator.hh"
#include "mms/structs/Structs.hh"

#include <fiah/utils/XorBitant.hh>
#include <fiah/io/Udp.hh>

namespace mms
{

void MarketDepthGenerator::start() noexcept
{
    m_run_thread = std::jthread{&MarketDepthGenerator::run<true>, this};
}

void MarketDepthGenerator::enqueue(const MarketDepthMessage& msg) noexcept
{
    m_send_q.push(msg);
}

MarketDepthMessage MarketDepthGenerator::generate_depth_message() noexcept
{
    std::uniform_int_distribution price_modifier{1000, 5000};
    std::uniform_int_distribution side_dist{0, 1};
    std::uniform_int_distribution idx_dist{0, static_cast<int>(m_symbols.size() - 1)};
    const auto idx = static_cast<std::size_t>(idx_dist(m_prng));
    const auto is_bid = static_cast<std::uint8_t>(side_dist(m_prng));
    Price price = is_bid?
        Price{m_base_prices[idx] - price_modifier(m_prng)}: 
        Price{m_base_prices[idx] + price_modifier(m_prng)};
    const std::uint8_t flags = is_bid & std::to_underlying(MarketDepthMessage::Flags::IsBidBit);


    return {
        _generate_header(),
        _generate_option(idx),
        price,
        0,
        flags 
    };
}


void MarketDepthGenerator::send_msg() noexcept
{
    MarketDepthMessage msg;
    if (!m_send_q.pop(msg))
        return;

    const auto buf = _serialize(msg);
    for (auto i{0uz}; i < m_clients.size(); ++i)
        m_udp.send(buf.data(), buf.size(), m_clients[i].get_sockaddr_in());
    ++m_seq;
}

Option MarketDepthGenerator::_generate_option(std::size_t idx) const noexcept
{
    // std::uniform_int_distribution is_call_dist{0, 1};
    return {
        m_strikes[idx],
        m_symbols[idx],
        26,
        6,
        19,
        // static_cast<std::uint8_t>(is_call_dist(xor_gen))
        static_cast<std::uint8_t>(1) // is_call = 1 hard coded
    };
}


MarketDepthMessage::Header MarketDepthGenerator::_generate_header() noexcept
{
    MarketDepthMessage::Header header{};
    header.version = WireDepthMsgV1::VERSION;
    header.seq = m_seq;
    header.ts_ns = fiah::TimeStamp{}.update_now().get_ticks();
    return header;
}

auto MarketDepthGenerator::_serialize(const MarketDepthMessage& msg) noexcept
    -> MarketDepthPacket
{
    MarketDepthPacket buf{};
    _write_u8(buf, WireMessage::TYPE_OFFSET, std::to_underlying(WireMessage::MessageType::Depth));
    _write_u8(buf, WireMessage::VERSION_OFFSET, WireDepthMsgV1::VERSION);
    _write_u16(buf, WireMessage::LENGTH_OFFSET, WireDepthMsgV1::LENGTH);

    _write_u64(buf, WireDepthMsgV1::TIMESTAMP_OFFSET, msg.header.ts_ns);
    _write_u16(buf, WireDepthMsgV1::SEQUENCE_OFFSET, msg.header.seq);

    std::memcpy(buf.data() + WireDepthMsgV1::SYMBOL_OFFSET, msg.option.symbol.data, Symbol::MAX_SYMBOL_SIZE);
    _write_u8(buf, WireDepthMsgV1::YEAR_OFFSET, msg.option.year);
    _write_u8(buf, WireDepthMsgV1::MONTH_OFFSET, msg.option.month);
    _write_u8(buf, WireDepthMsgV1::DAY_OFFSET, msg.option.day);
    _write_u8(buf, WireDepthMsgV1::FLAGS_OFFSET, msg.flags);
    _write_u32(buf, WireDepthMsgV1::STRIKE_OFFSET, static_cast<Price::type>(msg.option.strike));
    _write_u32(buf, WireDepthMsgV1::PRICE_OFFSET, static_cast<Price::type>(msg.price));

    return buf;
}

void MarketDepthGenerator::_write_u8(MarketDepthPacket& buf, std::size_t offset, std::uint8_t val) noexcept
{
    buf[offset] = static_cast<std::byte>(val);
}

void MarketDepthGenerator::_write_u16(MarketDepthPacket& buf, std::size_t offset, std::uint16_t val) noexcept
{
    buf[offset] = static_cast<std::byte>((val >> 8) & 0xFF);
    buf[offset + 1] = static_cast<std::byte>(val & 0xFF);
}

void MarketDepthGenerator::_write_u32(MarketDepthPacket& buf, std::size_t offset, std::uint32_t val) noexcept
{
    buf[offset] = static_cast<std::byte>((val >> 24) & 0xFF);
    buf[offset + 1] = static_cast<std::byte>((val >> 16) & 0xFF);
    buf[offset + 2] = static_cast<std::byte>((val >> 8) & 0xFF);
    buf[offset + 3] = static_cast<std::byte>(val & 0xFF);
}

void MarketDepthGenerator::_write_u64(MarketDepthPacket& buf, std::size_t offset, std::uint64_t val) noexcept
{
    buf[offset] = static_cast<std::byte>((val >> 56) & 0xFF);
    buf[offset + 1] = static_cast<std::byte>((val >> 48) & 0xFF);
    buf[offset + 2] = static_cast<std::byte>((val >> 40) & 0xFF);
    buf[offset + 3] = static_cast<std::byte>((val >> 32) & 0xFF);
    buf[offset + 4] = static_cast<std::byte>((val >> 24) & 0xFF);
    buf[offset + 5] = static_cast<std::byte>((val >> 16) & 0xFF);
    buf[offset + 6] = static_cast<std::byte>((val >> 8) & 0xFF);
    buf[offset + 7] = static_cast<std::byte>(val & 0xFF);
}
} // End namespace mms
