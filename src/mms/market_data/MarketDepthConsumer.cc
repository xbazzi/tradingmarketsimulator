#include <cstring>

#include "mms/market_data/MarketDepthConsumer.hh"
#include <fiah/thread/ThreadPool.hpp>
#include <fiah/error/Error.hh>

namespace mms
{

std::expected<void, Error> MarketDepthConsumer::run() noexcept
{
    std::byte buf[DEPTH_MSG_LEN];
    while(true)
    {
        if (auto msg = read_msg(buf); msg)
            m_recv_queue.push(*msg);
    }
}

std::expected<MarketDepthMessage, fiah::UdpError> MarketDepthConsumer::read_msg(std::byte* buf) noexcept
{
    sockaddr_in peer{};
    return m_udp_client.recv(buf, DEPTH_MSG_LEN, peer)
        .transform([buf](std::size_t read) {
            MarketDepthMessage msg;
            std::memcpy(&msg, buf, DEPTH_MSG_LEN);
            return msg;
        });
}

} // End namespace mms
