#pragma once

// C++ Includes
#include <cstddef>
#include <expected>
#include <string>

// Third party
#include <fiah/io/Udp.hh>
#include <fiah/structs/SPSCQueue.hh>
#include <fiah/utils/Types.hh>

// MMS Includes
#include "mms/error/Error.hh"
#include "mms/structs/Structs.hh"

using namespace fiah;

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
    u8_t _read_u8(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;
    u16_t _read_u16(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;
    u32_t _read_u32(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;
    u64_t _read_u64(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;

    i8_t _read_i8(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;
    i16_t _read_i16(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;
    i32_t _read_i32(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;
    i64_t _read_i64(const WireDepthMsgV1::Buffer& buf, std::size_t offset) noexcept;

private:
    fiah::SPSCQueue<InternalDepthMessage, 1 << 10> m_recv_queue;
    fiah::UdpClient m_udp_client;
    std::jthread m_recv_thread;
};
} // End namespace mms
