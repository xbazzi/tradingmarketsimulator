#pragma once

// C++ Includes
#include <cstddef>
#include <expected>
#include <string>

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

    MarketDepthConsumer() = delete;
    MarketDepthConsumer(fiah::UdpClient udp_client) noexcept
        : m_udp_client(std::move(udp_client)) ,
          m_recv_thread {std::jthread{&MarketDepthConsumer::run, this}} {}
    ~MarketDepthConsumer() = default;

    void run() noexcept;
    ssize_t read_msg(WireDepthMsgV1::Buffer& buf) noexcept;
    bool try_pop(InternalDepthMessage& out) noexcept;

private:
    InternalDepthMessage _deserialize(const WireDepthMsgV1::Buffer& buf) noexcept;
    std::uint8_t _read_u8(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;
    std::uint16_t _read_u16(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;
    std::uint32_t _read_u32(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;
    std::uint64_t _read_u64(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;

private:
    fiah::SPSCQueue<InternalDepthMessage, 1 << 10> m_recv_queue;
    fiah::UdpClient m_udp_client;
    std::jthread m_recv_thread;
};
} // End namespace mms
