#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <thread>
#include <string>
#include <format>


// Third Party Includes
#include <fiah/utils/TimeStamp.hh>
#include <fiah/structs/SPSCQueue.hh>
#include <fiah/utils/Types.hh>


// MarketMakerSimulator Includes
#include "mms/market/Price.hh"

using namespace fiah;

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
    u64_t seq_num;
    double bid;
    double ask;
    u64_t timestamp_ns;
    char symbol[8];
};
#pragma pack(pop)

/// @brief Market signal
struct Signal
{
    char symbol[8];
    enum class Type : u8_t
    {
        BUY,
        SELL,
        HOLD
    } type;
    double price;
    u64_t quantity;
    u64_t timestamp_ns;
};

    namespace v1
    {
    struct Order
    {
        char symbol[8];
        enum class Side : u8_t
        {
            BUY,
            SELL
        } side;
        double price;
        uint64_t quantity;
        uint64_t order_id;
        uint64_t timestamp_ns;
    };

    }

struct Symbol
{
    static constexpr u8_t MAX_SYMBOL_SIZE{6};
    char data[MAX_SYMBOL_SIZE];
};

    inline namespace v2
    {
    struct Order
    {
        Symbol symbol;
        enum class Side : u8_t { Bid, Ask };
        Side side;
        Price price;
        u32_t qty;
        u32_t id;
        u64_t rcv_ts;
        u8_t flags;

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

    }


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
        v1::Order order;
    };
};

struct Worker
{
    std::thread thread;
    fiah::SPSCQueue<Task, 1024> queue;
    std::atomic<bool> running{true};
    int cpu_affinity;
};


enum class Flags : u8_t
{
    IsCall = 0b1,
    Reserved = 0b11111110

};

struct Option
{
    enum class Flags : u8_t
    {
        IsCall = 0b0000'0001
    };
    Price strike;         
    Symbol symbol;        
    u8_t year;
    u8_t month;
    u8_t day;
    u8_t flags; // 0: is_call, 1-7: reserved
    u8_t _pad[2];
};

using TimeStamp = fiah::TimeStamp<>;

struct MarketDepthMessage
{
    enum class Flags : u8_t
    {
        IsBidBit = 0b0000'0001,
    };
    struct Header
    {
        TimeStamp::Rep ts_ns;
        u16_t seq;
        u8_t version;
        std::byte _reserved[5];
    } header;
    Option option;
    Price price;
    u8_t flags;
    u8_t _pad[1];
};
/// @todo switch to static reflection for member sizes
static_assert(sizeof(MarketDepthMessage) == sizeof(MarketDepthMessage::Header)
    + sizeof(Option) + sizeof(Price) + sizeof(u16_t) + sizeof(u8_t) + sizeof(u8_t));
static_assert(offsetof(MarketDepthMessage, option) % alignof(Option) == 0);
static_assert(offsetof(MarketDepthMessage, price) % alignof(Price) == 0);
static_assert(offsetof(MarketDepthMessage::Header, seq) % alignof(u16_t) == 0);

struct InternalDepthMessage
{
    Option option;
    TimeStamp::Rep market_ts_ns;
    TimeStamp::Rep local_ts;
    Price price;
    u16_t seq;
    u8_t flags;
    enum class Flags : u8_t
    {
        IsBid = 0b0000'0001
    }; 
};

struct WireMessage
{
    enum class MessageType : u8_t
    {
        None = 0,
        Depth = 1,
    };

    static constexpr auto TYPE_OFFSET = 0uz;
    static constexpr auto VERSION_OFFSET = TYPE_OFFSET + sizeof(u8_t);
    static constexpr auto LENGTH_OFFSET = VERSION_OFFSET + sizeof(u8_t);
    static constexpr auto HEADER_SIZE = LENGTH_OFFSET + sizeof(u16_t);
};

struct WireDepthMsgV1
{
    static constexpr WireMessage::MessageType TYPE{WireMessage::MessageType::Depth};
    static constexpr u8_t VERSION = 1;

    static constexpr auto TIMESTAMP_OFFSET = WireMessage::HEADER_SIZE;
    static constexpr auto SEQUENCE_OFFSET  = TIMESTAMP_OFFSET + sizeof(u64_t);
    static constexpr auto SYMBOL_OFFSET    = SEQUENCE_OFFSET + sizeof(u16_t);
    static constexpr auto YEAR_OFFSET      = SYMBOL_OFFSET + Symbol::MAX_SYMBOL_SIZE;
    static constexpr auto MONTH_OFFSET     = YEAR_OFFSET + sizeof(u8_t);
    static constexpr auto DAY_OFFSET       = MONTH_OFFSET + sizeof(u8_t);
    static constexpr auto FLAGS_OFFSET     = DAY_OFFSET + sizeof(u8_t);
    static constexpr auto STRIKE_OFFSET    = FLAGS_OFFSET + sizeof(u8_t);
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

constexpr sz_t MARKET_DEPTH_MSG_LEN{sizeof(MarketDepthMessage)};
constexpr sz_t INTERNAL_DEPTH_MSG_LEN{sizeof(InternalDepthMessage)};


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