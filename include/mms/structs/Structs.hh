#pragma once

#include <cstdint>
#include <thread>

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

// #pragma pack(push, 1)
/// @brief Incoming data from market
struct MarketData
{
    std::uint64_t seq_num;
    double bid;
    double ask;
    std::uint64_t timestamp_ns;
    char symbol[8];
};
// #pragma pack(pop)

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

struct Order
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
        Order order;
    };
};

struct Worker
{
    std::thread thread;
    fiah::SPSCQueue<Task, 1024> queue;
    std::atomic<bool> running{true};
    int cpu_affinity;
};

constexpr std::uint8_t MAX_SYMBOL_SIZE{6};
struct Option
{
    Price strike;
    char symbol[MAX_SYMBOL_SIZE];
    std::uint8_t year;
    std::uint8_t month;
    std::uint8_t day;
};

using TimeStamp = fiah::TimeStamp<>;

struct DepthMessage
{
    Option option;
    TimeStamp ts;
    std::uint16_t seq;
};

} // End namespace mms