// C++ Includes
#include <chrono>
#include <cstring>
#include <thread>

// MarketMakerSimulator Includes
#include "mms/error/CoreException.hh"
#include "mms/market/MarketFeed.hh"
#include <fiah/utils/Timer.hh>

namespace mms
{

MarketFeed::MarketFeed(const fiah::Config &config, fiah::SPSCQueue<MarketData, 4096UL> &queue)
    : m_config(config), m_market_data_queue(queue)
{
    LOG_DEBUG("MarketFeed constructed");
}

MarketFeed::~MarketFeed()
{
    stop();
    LOG_DEBUG("MarketFeed destroyed");
}

auto MarketFeed::initialize() -> std::expected<void, CoreError>
{
    fiah::Timer timer{};

    if (is_initialized())
    {
        LOG_WARN("MarketFeed already initialized");
        return {};
    }

    const std::string &market_ip = m_config.get_market_ip();
    std::uint16_t market_port = m_config.get_market_port();

    p_tcp_client = fiah::make_unique<fiah::TcpClient>(market_ip, market_port);

    LOG_INFO("MarketFeed initialized with market server ip: ", market_ip, ", and port: ", market_port);

    // Connect immediately if possible
    auto connect_result = p_tcp_client->connect_to_server();
    if (!connect_result.has_value())
    {
        LOG_ERROR("Couldn't connect to market server during initialization. ", "Maybe the server is not online yet.");
        // Clean up the partially initialized client
        p_tcp_client.reset();
        m_initialized.store(false, std::memory_order_release);
        return std::unexpected(CoreError::SERVER_NOT_ONLINE);
    }

    m_initialized.store(true, std::memory_order_release);
    LOG_INFO("MarketFeed initialized and connected to market.");
    return {};
}

auto MarketFeed::_reconnect() -> std::expected<void, CoreError>
{
    LOG_INFO("Attempting to reconnect MarketFeed...");

    const std::string &market_ip = m_config.get_market_ip();
    std::uint16_t market_port = m_config.get_market_port();

    // Clean up old client
    p_tcp_client.reset();
    m_initialized.store(false, std::memory_order_release);

    // Create new client and attempt connection
    p_tcp_client = fiah::make_unique<fiah::TcpClient>(market_ip, market_port);

    auto connect_result = p_tcp_client->connect_to_server();
    if (!connect_result.has_value())
    {
        LOG_WARN("Reconnection attempt failed - server not available");
        p_tcp_client.reset();
        return std::unexpected{CoreError::SERVER_NOT_ONLINE};
    }

    m_initialized.store(true, std::memory_order_release);
    LOG_INFO("MarketFeed successfully reconnected to market.");
    return {};
}

void MarketFeed::receive_loop(std::atomic<bool> &running_flag)
{
    using namespace std::chrono_literals;

    try
    {
        LOG_INFO("MarketFeed receive loop started");
        std::byte buffer[sizeof(MarketData)];

        while (running_flag.load(std::memory_order_acquire))
        {
            fiah::Timer timer{};

            // Critical: Validate p_tcp_client exists before dereferencing
            if (!p_tcp_client)
            {
                LOG_WARN("TCP client is null - attempting reconnection...");

                // Attempt to reconnect with exponential backoff
                int retry_count = 0;
                const int max_retries = 5;

                while (retry_count < max_retries && running_flag.load(std::memory_order_acquire))
                {
                    auto reconnect_result = _reconnect();
                    if (reconnect_result.has_value())
                    {
                        LOG_INFO("Reconnection successful!");
                        break;
                    }

                    retry_count++;
                    if (retry_count < max_retries)
                    {
                        auto backoff = std::chrono::milliseconds(100 * (1 << retry_count));
                        LOG_WARN("Reconnection attempt ", retry_count, " failed. Retrying in ", backoff.count(),
                                 "ms...");
                        std::this_thread::sleep_for(backoff);
                    }
                }

                if (!p_tcp_client)
                {
                    LOG_ERROR("Failed to reconnect after ", max_retries, " attempts. Thread exiting.");
                    running_flag.store(false, std::memory_order_release);
                    return;
                }

                continue; // Retry recv with new connection
            }

            auto recv_result = p_tcp_client->recv(buffer, sizeof(buffer));
            if (!recv_result.has_value())
            {
                LOG_WARN("Failed to receive market data packet. "
                         "Socket disconnected - will attempt reconnect.");
                // Don't exit - clean up and let the null check above handle
                // reconnection
                p_tcp_client.reset();
                m_initialized.store(false, std::memory_order_release);
                continue;
            }

            if (recv_result.value() != sizeof(MarketData))
            {
                LOG_WARN("Received incomplete market data packet. Discarding...");
                continue;
            }

            MarketData md;
            std::memcpy(&md, buffer, sizeof(MarketData));
            m_ticks_received.fetch_add(1, std::memory_order_relaxed);

            LOG_DEBUG("Got md (raw): ", "Symbol: ", md.symbol, ", ", "Seq: ", md.seq_num, ", ", "Ask: ", md.ask, ", ",
                      "Bid: ", md.bid, ", ", "Timestamp: ", static_cast<uint64_t>(md.timestamp_ns), ", ",
                      "ticksReceived: ", m_ticks_received.load(std::memory_order_acquire));

            if (!m_market_data_queue.push(md))
            {
                // Queue full
                m_queue_full_count.fetch_add(1, std::memory_order_relaxed);

                // Try to push with timeout to avoid infinite spin if shutting down
                while (running_flag.load(std::memory_order_acquire))
                {
                    if (m_market_data_queue.push(md))
                        break;
                    std::this_thread::yield();
                }
            }
        }
        LOG_INFO("MarketFeed receive loop exiting...");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("MarketFeed receive loop crashed with exception: ", e.what());
        running_flag.store(false, std::memory_order_release);
    }
    catch (...)
    {
        LOG_ERROR("MarketFeed receive loop crashed with unknown exception");
        running_flag.store(false, std::memory_order_release);
    }
}

void MarketFeed::stop()
{
    LOG_INFO("Stopping MarketFeed...");

    // Clean up client connection
    if (p_tcp_client)
    {
        p_tcp_client.reset();
    }

    m_initialized.store(false, std::memory_order_release);

    LOG_INFO("MarketFeed stopped. Stats: ", "Ticks received: ", m_ticks_received.load(std::memory_order_relaxed),
             ", Queue full events: ", m_queue_full_count.load(std::memory_order_relaxed));
}

} // namespace mms
