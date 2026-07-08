#include <cstring>
#include <cstddef>
#include <sys/types.h>

#include "mms/market_data/MarketDepthConsumer.hh"
#include <fiah/error/Error.hh>
#include <fiah/utils/Types.hh>

using namespace fiah;

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

    /// @todo implement validation using these
    // const auto version = _read_u8(buf, WireMessage::VERSION_OFFSET);
    // const auto type = _read_u8(buf, WireMessage::TYPE_OFFSET);
    // const auto length = _read_u8(buf, WireMessage::LENGTH_OFFSET);

    msg.market_ts_ns = _read_u64(buf, WireDepthMsgV1::TIMESTAMP_OFFSET);
    msg.seq = _read_u16(buf, WireDepthMsgV1::SEQUENCE_OFFSET);
    std::memcpy(&msg.option.symbol, buf.data() + WireDepthMsgV1::SYMBOL_OFFSET, Symbol::MAX_SYMBOL_SIZE);
    msg.option.year = _read_u8(buf, WireDepthMsgV1::YEAR_OFFSET);
    msg.option.month = _read_u8(buf, WireDepthMsgV1::MONTH_OFFSET);
    msg.option.day = _read_u8(buf, WireDepthMsgV1::DAY_OFFSET);
    msg.flags = _read_u8(buf, WireDepthMsgV1::FLAGS_OFFSET);

    msg.option.strike = Price{_read_i32(buf, WireDepthMsgV1::STRIKE_OFFSET)}; 
    msg.price = Price{_read_i32(buf, WireDepthMsgV1::PRICE_OFFSET)};

    msg.local_ts = TimeStamp{}.update_now().get_ticks();
    return msg;
}

u8_t MarketDepthConsumer::_read_u8(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    return static_cast<u8_t>(buf[offset]);
}

u16_t MarketDepthConsumer::_read_u16(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    auto hi = std::to_integer<u8_t>(buf[offset]);
    auto lo = std::to_integer<u8_t>(buf[offset + 1]);
    return (static_cast<u16_t>(hi) << 8u) | lo;
}

u32_t MarketDepthConsumer::_read_u32(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    auto hi = _read_u16(buf, offset);
    auto lo = _read_u16(buf, offset + sizeof(u16_t));
    return (static_cast<u32_t>(hi) << 16u) | lo;
}

u64_t MarketDepthConsumer::_read_u64(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    auto hi = _read_u32(buf, offset);
    auto lo = _read_u32(buf, offset + sizeof(u32_t));
    return (static_cast<u64_t>(hi) << 32ull) | lo;
}

i8_t MarketDepthConsumer::_read_i8(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    return static_cast<i8_t>(_read_u8(buf, offset));
}

i16_t MarketDepthConsumer::_read_i16(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    return static_cast<i16_t>(_read_u16(buf, offset));
}

i32_t MarketDepthConsumer::_read_i32(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    return static_cast<i32_t>(_read_u32(buf, offset));
}

i64_t MarketDepthConsumer::_read_i64(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept
{
    return static_cast<i64_t>(_read_u64(buf, offset));
}
} // End namespace mms
