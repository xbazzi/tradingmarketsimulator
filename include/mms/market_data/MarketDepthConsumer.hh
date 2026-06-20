#pragma once

// C++ Includes
#include <cstdint>
#include <expected>

// Third party
#include <fiah/io/Udp.hh>
#include <fiah/structs/SPSCQueue.hh>

// MMS Includes
#include "mms/error/Error.hh"
#include "mms/structs/Structs.hh"

namespace mms
{
class MarketDepthConsumer
{
public:
    static constexpr std::size_t DEPTH_MSG_LEN{sizeof(MarketDepthMessage)};

    MarketDepthConsumer() = delete;
    MarketDepthConsumer(fiah::UdpClient udp_client)
        : m_udp_client(std::move(udp_client)) 
    {
        m_recv_thread = std::jthread{&MarketDepthConsumer::run, this};
    }
    ~MarketDepthConsumer() = default;

    std::expected<void, Error> run() noexcept;
    std::expected<MarketDepthMessage, fiah::UdpError> read_msg(std::byte* buf) noexcept;
    bool try_pop(MarketDepthMessage& out) noexcept { return m_recv_queue.pop(out); }

private:
    fiah::SPSCQueue<MarketDepthMessage, 1 << 10> m_recv_queue;
    fiah::UdpClient m_udp_client;
    std::jthread m_recv_thread;
};
} // End namespace mms
