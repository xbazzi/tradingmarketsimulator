#include <cstring>
#include <cstddef>
#include <sys/types.h>

#include "mms/market_data/MarketDepthConsumer.hh"
#include <fiah/error/Error.hh>

namespace mms
{

void MarketDepthConsumer::run() noexcept
{
    WireDepthMsgV1::Buffer buf{};
    while(true)
    {
        if (auto read_bytes = read_msg(buf); read_bytes)
        {
            auto msg = _deserialize(buf);
            m_recv_queue.push(std::move(msg));
        }
    }
}

ssize_t MarketDepthConsumer::read_msg(WireDepthMsgV1::Buffer& buf) noexcept
{
    sockaddr_in peer{};
    return m_udp_client.recv(buf.data(), MARKET_DEPTH_MSG_LEN, peer);
}


bool MarketDepthConsumer::try_pop(InternalDepthMessage& out) noexcept
{
    return m_recv_queue.pop(out); 
}

/// @todo use cpp26 static reflection instead
InternalDepthMessage MarketDepthConsumer::_deserialize(const WireDepthMsgV1::Buffer& buf) noexcept
{
    InternalDepthMessage msg{};
    const auto version = _read_u8(buf, WireMessage::VERSION_OFFSET);
    const auto type = _read_u8(buf, WireMessage::TYPE_OFFSET);
    const auto length = _read_u8(buf, WireMessage::LENGTH_OFFSET);

    msg.market_ts_ns = _read_u64(buf, WireDepthMsgV1::TIMESTAMP_OFFSET);
    msg.seq = _read_u16(buf, WireDepthMsgV1::SEQUENCE_OFFSET);
    std::memcpy(&msg.option.symbol, buf.data() + WireDepthMsgV1::SYMBOL_OFFSET, Symbol::MAX_SYMBOL_SIZE);
    msg.option.year = _read_u8(buf, WireDepthMsgV1::YEAR_OFFSET);
    msg.option.month = _read_u8(buf, WireDepthMsgV1::MONTH_OFFSET);
    msg.option.day = _read_u8(buf, WireDepthMsgV1::DAY_OFFSET);
    msg.flags = _read_u8(buf, WireDepthMsgV1::FLAGS_OFFSET);

    /// @todo change these to read_i32
    msg.option.strike = Price{_read_u32(buf, WireDepthMsgV1::STRIKE_OFFSET)}; 
    msg.price = Price{_read_u32(buf, WireDepthMsgV1::PRICE_OFFSET)};

    msg.local_ts = TimeStamp{}.update_now().get_ticks();
    return msg;
}

std::uint8_t MarketDepthConsumer::_read_u8(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    return static_cast<std::uint8_t>(buf[offset]);
}

std::uint16_t MarketDepthConsumer::_read_u16(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    auto hi = std::to_integer<std::uint8_t>(buf[offset]);
    auto lo = std::to_integer<std::uint8_t>(buf[offset + 1]);
    return (static_cast<std::uint16_t>(hi) << 8u) | lo;
}

std::uint32_t MarketDepthConsumer::_read_u32(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    auto hi = _read_u16(buf, offset);
    auto lo = _read_u16(buf, offset + sizeof(std::uint16_t));
    return (static_cast<std::uint32_t>(hi) << 16u) | lo;
}

std::uint64_t MarketDepthConsumer::_read_u64(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    auto hi = _read_u32(buf, offset);
    auto lo = _read_u32(buf, offset + sizeof(std::uint32_t));
    return (static_cast<std::uint64_t>(hi) << 32ull) | lo;
}
} // End namespace mms
