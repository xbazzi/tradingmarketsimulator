#pragma once

// C++ Includes
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <expected>
#include <thread>

// MarketMakerSimulator includes
#include "mms/error/Error.hh"
#include "mms/structs/Structs.hh"
#include "quick/handle/UniquePtr.hh"
#include "quick/io/Config.hh"
#include "quick/io/TcpClient.hh"
#include "quick/structs/SPSCQueue.hh"
#include "quick/utils/Logger.hh"
#include "quick/utils/Timer.hh"

namespace mms::io
{

/// @brief Handles incoming market data from the network
///
/// This class encapsulates all logic related to receiving market data:
/// - TCP connection lifecycle (connect, disconnect, reconnect)
/// - Receiving raw bytes and parsing into MarketData structs
/// - Pushing data into a lock-free queue for downstream processing
/// - Connection recovery with exponential backoff
/// - Performance metrics tracking
///
/// @section thread_safety Thread Safety
/// - receive_loop() is designed to be called from a dedicated thread
/// - All public methods are thread-safe through atomic operations
/// - The queue reference must remain valid for the lifetime of this object
///
/// @section error_handling Error Handling
/// - initialize() returns std::expected for recoverable connection errors
/// - receive_loop() catches all exceptions and logs them (never throws)
/// - Connection failures trigger automatic reconnection logic
class MarketFeed
{
  private:
    using MarketData = structs::MarketData;
    using TcpClientUniquePtr = quick::handle::UniquePtr<quick::io::TcpClient>;

    /// @brief Configuration reference
    const quick::Config &m_config;

    /// @brief TCP client for receiving market data
    TcpClientUniquePtr p_tcp_client;

    /// @brief Reference to market data queue (owned by caller)
    quick::structs::SPSCQueue<MarketData, 4096UL> &m_market_data_queue;

    /// @brief Initialization state
    std::atomic<bool> m_initialized{false};

    /// @brief Performance counter: total ticks received
    alignas(64) std::atomic<uint64_t> m_ticks_received{0};

    /// @brief Performance counter: queue full events
    alignas(64) std::atomic<uint64_t> m_queue_full_count{0};

    /// @brief Logger instance
    static inline quick::utils::Logger<MarketFeed> &m_logger{
        quick::utils::Logger<MarketFeed>::get_instance("MarketFeed")};

    /// @brief Internal reconnection logic
    /// @return Success or CoreError::SERVER_NOT_ONLINE
    std::expected<void, CoreError> _reconnect();

  public:
    /// @brief Construct a MarketFeed
    /// @param config Configuration containing market server details
    /// @param queue Reference to the market data queue to push into
    explicit MarketFeed(const quick::Config &config, quick::structs::SPSCQueue<MarketData, 4096UL> &queue);

    /// @brief Destructor
    ~MarketFeed();

    // Delete copy/move to avoid queue reference issues
    MarketFeed(const MarketFeed &) = delete;
    MarketFeed &operator=(const MarketFeed &) = delete;
    MarketFeed(MarketFeed &&) = delete;
    MarketFeed &operator=(MarketFeed &&) = delete;

    /// @brief Initialize and connect to the market server
    /// @return Success or CoreError::SERVER_NOT_ONLINE
    std::expected<void, CoreError> initialize();

    /// @brief Main receive loop - call from a dedicated thread
    /// @param running_flag Atomic flag to control loop lifetime
    ///
    /// This method:
    /// - Receives raw bytes from TCP socket
    /// - Parses into MarketData structs
    /// - Pushes to the queue
    /// - Handles reconnection on connection loss
    /// - Never throws (catches all exceptions)
    void receive_loop(std::atomic<bool> &running_flag);

    /// @brief Stop the feed and clean up resources
    void stop();

    /// @brief Get total ticks received
    __always_inline std::uint64_t ticks_received() const noexcept
    {
        return m_ticks_received.load(std::memory_order_relaxed);
    }

    /// @brief Get total queue full events
    __always_inline uint64_t queue_full_count() const noexcept
    {
        return m_queue_full_count.load(std::memory_order_relaxed);
    }

    /// @brief Check if feed is initialized and connected
    __always_inline bool is_initialized() const noexcept
    {
        return m_initialized.load(std::memory_order_relaxed);
    }
};

} // namespace mms::io
