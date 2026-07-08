#ifndef MMS_ALGO_HH
#define MMS_ALGO_HH

// C++ Includes
// #include <memory>
#include <atomic>
#include <cstdint>
#include <future>
#include <thread>

// MarketMakerSimulator includes
#include "mms/error/CoreException.hh"
#include "mms/error/Error.hh"
#include "mms/market/MarketFeed.hh"
#include "mms/structs/Structs.hh"

// Third-Party Includes
#include <fiah/handle/UniquePtr.hh>
#include <fiah/io/Config.hh>
#include <fiah/io/TcpServer.hh>
#include <fiah/structs/SPSCQueue.hh>
#include <fiah/structs/Vector.hh>
#include <fiah/utils/Logger.hh>

namespace mms
{

/// @brief Handles orchestration of the entire system
///
/// @section error_handling Error Handling Strategy
///
/// This class uses a hybrid error handling approach:
///
/// @subsection std_expected std::expected<void, CoreError>
/// Used for **recoverable errors** and **expected failure modes**:
/// - Network unavailable (server not online, connection timeout)
/// - Initialization failures (port in use, permission denied)
/// - Resource temporarily unavailable
///
/// Functions returning std::expected:
/// - initialize_client() - Connection may fail if server offline
/// - initialize_server() - Bind may fail if port in use
/// - work_client() - Thread startup or initialization may fail
/// - work_server() - Server operations may fail gracefully
///
/// @subsection exceptions Exceptions (CoreException, std::exception)
/// Used for **unrecoverable errors** and **programming errors**:
/// - Invariant violations (null pointers after successful initialization)
/// - Invalid state transitions (using uninitialized objects)
/// - System resource exhaustion (thread creation failure)
/// - Fatal errors that must propagate to Controller
///
/// Functions that may throw:
/// - All public functions may throw CoreException for fatal errors
/// - Controller catches all exceptions as the termination point
///
/// @subsection thread_safety Thread Safety
/// - Thread loop functions (_network_loop, _strategy_loop, _execution_loop)
///   NEVER throw - they catch all exceptions internally and log them
/// - Atomic flags (m_client_running, etc.) provide thread-safe state
///
/// @attention Always catch exceptions when calling Core methods from
/// Controller!
class Core
{

  private:
    /// @brief Drop-in replacement for std::unique_ptr<> and std::make_unique<>
    using ConfigUniquePtr = fiah::UniquePtr<fiah::Config>;
    using TcpServerUniquePtr = fiah::UniquePtr<fiah::TcpServer>;
    using MarketFeedUniquePtr = fiah::UniquePtr<MarketFeed>;

    ConfigUniquePtr p_config;
    static inline fiah::Logger<Core> &m_logger{fiah::Logger<Core>::get_instance("Core")};

    std::atomic<bool> m_server_started{false};
    std::atomic<bool> m_client_started{false};
    std::atomic<bool> m_client_stopping{false};
    std::atomic<bool> m_client_stopped{false};
    std::atomic<bool> m_client_running{false};

    // Lock-free single-producer, single-consumer queues
    fiah::SPSCQueue<MarketData, 4096UL> m_market_data_queue;
    fiah::SPSCQueue<Signal, 2048UL> m_signal_queue;
    fiah::SPSCQueue<Order, 2048UL> m_order_queue;

    /// @brief Receives market data
    std::jthread m_network_thread;

    /// @brief Processes signals with a strategy
    std::jthread m_strategy_thread;

    /// @brief Sends orders
    std::jthread m_execution_thread;

    /// @brief Reads market data from JSON
    std::jthread m_reader_thread;

    /// @brief TCP server handle
    TcpServerUniquePtr p_tcp_server;

    /// @brief Market data feed handler
    MarketFeedUniquePtr p_market_feed;

    /// @brief Performance counters (lock-free)
    alignas(64) std::atomic<uint64_t> m_signals_generated{0};
    alignas(64) std::atomic<uint64_t> m_orders_sent{0};

    /// @brief Thread functions
    void _network_loop();
    void _strategy_loop();
    void _execution_loop();

    // Strategy logic
    Signal _compute_signal(const MarketData &md);
    v1::Order _generate_order(const Signal &signal);

    /// @brief Attempt to set thread affinity. Prints logs if it was
    /// unsuccessful.
    /// @param thread
    /// @param cpu_id
    void _set_thread_affinity(std::thread::native_handle_type thread, int cpu_id);

  public:
    explicit Core(fiah::Config &&);
    ~Core();

    std::expected<void, CoreError> initialize_client();
    std::expected<void, CoreError> initialize_server();

    [[nodiscard]] __always_inline bool is_server_initialized() const noexcept
    {
        return m_server_started.load(std::memory_order_relaxed);
    }

    [[nodiscard]] __always_inline bool is_client_initialized() const noexcept
    {
        return m_client_started.load(std::memory_order_relaxed);
    }

    [[nodiscard]] __always_inline bool is_client_running() const noexcept
    {
        return m_client_running.load(std::memory_order_relaxed);
    }

    /// Same as `__always_inline` GCC helper macro
    inline __attribute__((__always_inline__)) bool is_client_stopped() const noexcept
    {
        return m_client_stopped.load(std::memory_order_relaxed);
    }

    MarketData generate_market_data(std::string);

    std::expected<void, CoreError> work_server();
    std::expected<void, CoreError> work_client();
    void stop_client();
    void print_client_stats() const;
};
} // End namespace mms

#endif // ALGO_HH
