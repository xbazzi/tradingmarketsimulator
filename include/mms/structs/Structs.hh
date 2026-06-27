#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <thread>
#include <string>
#include <format>

#include "mms/market/Price.hh"
#include "fiah/utils/TimeStamp.hh"

// Third Party Includes

// MarketMakerSimulator Includes
#include "fiah/structs/SPSCQueue.hh"

namespace mms
{

///
/// @brief General purpose structs
/// @todo remove magic numbers
/// @todo Align to 64 bytes? -> (measure first)
///

#pragma pack(push, 1)
struct MarketData
{
    std::uint64_t seq_num;
    double bid;
    double ask;
    std::uint64_t timestamp_ns;
    char symbol[8];
};
#pragma pack(pop)

/// @brief Market signal
struct Signal
{
    char symbol[8];
    enum class Type : std::uint8_t
    {
        BUY,
        SELL,
        HOLD
    } type;
    double price;
    std::uint64_t quantity;
    std::uint64_t timestamp_ns;
};

struct OrderV1
{
    char symbol[8];
    enum class Side : std::uint8_t
    {
        BUY,
        SELL
    } side;
    double price;
    uint64_t quantity;
    uint64_t order_id;
    uint64_t timestamp_ns;
};

struct Symbol
{
    static constexpr std::uint8_t MAX_SYMBOL_SIZE{6};
    char data[MAX_SYMBOL_SIZE];
};

struct Order
{
    Symbol symbol;
    enum class Side : std::uint8_t { Bid, Ask };
    Side side;
    Price price;
    std::uint32_t qty;
    std::uint32_t id;
    std::uint64_t rcv_ts;
    std::uint8_t flags;

    constexpr Side opp_side() { return side == Side::Bid? Side::Ask: Side::Bid; };
    constexpr bool is_valid() 
    { 
        const bool valid_price = price.is_valid();
        const bool valid_qty = qty > 0;
        const bool valid_order = valid_price & valid_qty;
        return valid_order;
    };
    constexpr bool is_bid() { return side == Side::Bid? true: false; };
};


struct Task
{
    enum class Type
    {
        MarketData,
        Signal,
        Order,
        Risk
    };
    Type type;
    union {
        MarketData market_data;
        Signal signal;
        OrderV1 order;
    };
};

struct Worker
{
    std::thread thread;
    fiah::SPSCQueue<Task, 1024> queue;
    std::atomic<bool> running{true};
    int cpu_affinity;
};


enum class Flags : std::uint8_t
{
    IsCall = 0b1,
    Reserved = 0b11111110

};

struct Option
{
    enum class Flags : std::uint8_t
    {
        IsCall = 0b0000'0001
    };
    Price strike;         
    Symbol symbol;        
    std::uint8_t year;
    std::uint8_t month;
    std::uint8_t day;
    std::uint8_t flags; // 0: is_call, 1-7: reserved
    std::uint8_t _pad[2];
};

using TimeStamp = fiah::TimeStamp<>;

struct MarketDepthMessage
{
    enum class Flags : std::uint8_t
    {
        IsBidBit = 0b0000'0001,
    };
    struct Header
    {
        TimeStamp::rep ts_ns;
        std::uint16_t seq;
        std::uint8_t version;
        std::byte _reserved[5];
    } header;
    Option option;
    Price price;
    std::uint8_t flags;
    std::uint8_t _pad[1];
};
/// @todo switch to static reflection for member sizes
static_assert(sizeof(MarketDepthMessage) == sizeof(MarketDepthMessage::Header)
    + sizeof(Option) + sizeof(Price) + sizeof(std::uint16_t) + sizeof(std::uint8_t) + sizeof(std::uint8_t));
static_assert(offsetof(MarketDepthMessage, option) % alignof(Option) == 0);
static_assert(offsetof(MarketDepthMessage, price) % alignof(Price) == 0);
static_assert(offsetof(MarketDepthMessage::Header, seq) % alignof(std::uint16_t) == 0);

struct InternalDepthMessage
{
    Option option;
    TimeStamp::rep market_ts_ns;
    TimeStamp::rep local_ts;
    Price price;
    std::uint16_t seq;
    std::uint8_t flags;
    enum class Flags : std::uint8_t
    {
        IsBid = 0b0000'0001
    }; 
};

struct WireMessage
{
    enum class MessageType : std::uint8_t
    {
        None = 0,
        Depth = 1,
    };

    static constexpr auto TYPE_OFFSET = 0uz;
    static constexpr auto VERSION_OFFSET = TYPE_OFFSET + sizeof(std::uint8_t);
    static constexpr auto LENGTH_OFFSET = VERSION_OFFSET + sizeof(std::uint8_t);
    static constexpr auto HEADER_SIZE = LENGTH_OFFSET + sizeof(std::uint16_t);
};

struct WireDepthMsgV1
{
    static constexpr WireMessage::MessageType TYPE{WireMessage::MessageType::Depth};
    static constexpr std::uint8_t VERSION = 1;

    static constexpr auto TIMESTAMP_OFFSET = WireMessage::HEADER_SIZE;
    static constexpr auto SEQUENCE_OFFSET  = TIMESTAMP_OFFSET + sizeof(std::uint64_t);
    static constexpr auto SYMBOL_OFFSET    = SEQUENCE_OFFSET + sizeof(std::uint16_t);
    static constexpr auto YEAR_OFFSET      = SYMBOL_OFFSET + Symbol::MAX_SYMBOL_SIZE;
    static constexpr auto MONTH_OFFSET     = YEAR_OFFSET + sizeof(std::uint8_t);
    static constexpr auto DAY_OFFSET       = MONTH_OFFSET + sizeof(std::uint8_t);
    static constexpr auto FLAGS_OFFSET     = DAY_OFFSET + sizeof(std::uint8_t);
    static constexpr auto STRIKE_OFFSET    = FLAGS_OFFSET + sizeof(std::uint8_t);
    static constexpr auto PRICE_OFFSET     = STRIKE_OFFSET + sizeof(std::int32_t);

    static constexpr auto SIZE         = PRICE_OFFSET + sizeof(Price::type);
    static constexpr auto PAYLOAD_SIZE = SIZE - WireMessage::HEADER_SIZE;
    static constexpr auto LENGTH       = SIZE;
    static constexpr auto payload_size = PAYLOAD_SIZE;
    static constexpr auto size         = SIZE;
    static constexpr auto length       = LENGTH;

    using Buffer = std::array<std::byte, SIZE>;
};

static_assert(WireMessage::HEADER_SIZE == 4);
static_assert(sizeof(Price::type) == sizeof(std::int32_t));
static_assert(WireDepthMsgV1::PRICE_OFFSET + sizeof(Price::type) == WireDepthMsgV1::SIZE);

constexpr std::size_t MARKET_DEPTH_MSG_LEN{sizeof(MarketDepthMessage)};
constexpr std::size_t INTERNAL_DEPTH_MSG_LEN{sizeof(InternalDepthMessage)};


} // End namespace mms

template <>
struct std::formatter<mms::Order>
{
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }
    auto format(const mms::Order& o, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "Order{{id={}, side={}, price={}, qty={}}}",
            o.id,
            o.side == mms::Order::Side::Bid ? "Bid" : "Ask",
            static_cast<mms::Price::type>(o.price),
            o.qty
        );
    }

};