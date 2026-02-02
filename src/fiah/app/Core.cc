// C++ Includes
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <random>
#include <sstream>
#include <stop_token>
#include <string>

// FastInAHurry Includes
#include "fiah/app/Core.hh"
#include "fiah/error/CoreException.hh"
#include "fiah/structs/Structs.hh"

// Third Party Includes
#include <quick/io/Config.hh>
#include <quick/io/JSONReader.hh>
#include <quick/structs/Vector.hh>
#include <quick/utils/Logger.hh>
#include <quick/utils/Timer.hh>

namespace fiah
{

Core::Core(quick::Config &&config) : p_config{quick::handle::make_unique<quick::Config>(std::move(config))}
{
}

Core::~Core()
{
    stop_client();
}

auto Core::initialize_server() -> std::expected<void, CoreError>
{
    quick::utils::Timer timer{"Core::initialize_server()"};

    if (is_server_initialized())
    {
        LOG_WARN("TCP server already initialized");
        return {};
    }

    // Invariant check: p_config must exist (programming error if null)
    if (!p_config) [[unlikely]]
    {
        throw error::CoreException("FATAL: p_config is null - Core object in invalid state", CoreError::INVALID_STATE);
    }

    std::string market_ip = p_config->get_market_ip();
    std::uint16_t market_port = p_config->get_market_port();

    p_tcp_server = quick::handle::make_unique<quick::io::TcpServer>(market_ip, market_port);

    auto result = p_tcp_server->start();
    if (!result.has_value())
    {
        LOG_ERROR("Core failed to start TCP server on ", market_ip, ':', std::to_string(market_port), ", with error: ");
        return std::unexpected(CoreError::INIT_SERVER_FAIL);
    }
    m_server_started.store(true, std::memory_order_release);
    return {};
}

auto Core::initialize_client() -> std::expected<void, CoreError>
{
    quick::utils::Timer timer{"Core::initialize_client()"};
    if (is_client_initialized())
    {
        LOG_WARN("MarketFeed already initialized");
        return {};
    }

    // Invariant check: p_config must exist (programming error if null)
    if (!p_config) [[unlikely]]
    {
        throw error::CoreException("FATAL: p_config is null - Core object in invalid state", CoreError::INVALID_STATE);
    }

    // Create MarketFeed with reference to our market data queue
    p_market_feed = quick::handle::make_unique<io::MarketFeed>(*p_config, m_market_data_queue);

    // Initialize and connect
    auto init_result = p_market_feed->initialize();
    if (!init_result.has_value())
    {
        LOG_ERROR("Couldn't initialize MarketFeed. ", "Maybe the server is not online yet.");
        p_market_feed.reset();
        m_client_started.store(false, std::memory_order_release);
        return std::unexpected(init_result.error());
    }

    m_client_started.store(true, std::memory_order_release);
    LOG_INFO("MarketFeed initialized and connected.");
    return {};
}

auto Core::work_client() -> std::expected<void, CoreError>
{
    quick::utils::Timer timer{"Core::work_client()"};

    if (is_client_running())
    {
        LOG_WARN("Client already running.");
        return {};
    }

    if (!is_client_initialized())
    {
        auto result = initialize_client();
        if (!result.has_value())
        {
            LOG_ERROR("Failed to initialize client while starting.");
            return std::unexpected(result.error());
        }
    }

    m_client_running.store(true, std::memory_order_release);
    m_client_stopping.store(false, std::memory_order_release);
    m_client_stopped.store(false, std::memory_order_release);
    LOG_INFO("Starting multithreaded pipeline...");

    m_network_thread = std::jthread([this](std::stop_token) { this->_network_loop(); });
    LOG_INFO("Started network thread with id: ", m_network_thread.get_id());

    m_strategy_thread = std::jthread([this](std::stop_token) { this->_strategy_loop(); });
    LOG_INFO("Started strategy thread with id: ", m_strategy_thread.get_id());

    m_execution_thread = std::jthread([this](std::stop_token) { this->_execution_loop(); });
    LOG_INFO("Started execution thread with id: ", m_execution_thread.get_id());

    // Attempt to set affinity (prints warnings if it can't)
    _set_thread_affinity(m_network_thread.native_handle(), 0);
    _set_thread_affinity(m_strategy_thread.native_handle(), 1);
    _set_thread_affinity(m_execution_thread.native_handle(), 2);

    return {};
}

///
/// @brief THREAD 1: Network Loop - Delegates to MarketFeed
///
void Core::_network_loop()
{
    try
    {
        LOG_INFO("Network thread started (Core 0)");

        // Invariant check: MarketFeed must exist
        if (!p_market_feed) [[unlikely]]
        {
            LOG_ERROR("FATAL: MarketFeed is null in network loop");
            m_client_running.store(false, std::memory_order_release);
            return;
        }

        // Delegate all market data receiving to MarketFeed
        p_market_feed->receive_loop(m_client_running);

        LOG_INFO("Network thread exiting...");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Network thread crashed with exception: ", e.what());
        m_client_running.store(false, std::memory_order_release);
    }
    catch (...)
    {
        LOG_ERROR("Network thread crashed with unknown exception");
        m_client_running.store(false, std::memory_order_release);
    }
}

void Core::_strategy_loop()
{
    try
    {
        LOG_INFO("Strategy thread started (Core 1)");
        MarketData md;

        while (m_client_running.load(std::memory_order_acquire))
        {
            if (m_market_data_queue.pop(md))
            {
                Signal signal = _compute_signal(md);
                if (signal.type != Signal::Type::HOLD)
                {
                    if (!m_signal_queue.push(signal))
                    {
                        LOG_WARN("Signal queue full for ", signal.symbol);
                        // dropping signal for now
                    }
                    else
                    {
                        LOG_DEBUG("Pushed into signal queue. ", signal.price, ", ", signal.quantity, ", ",
                                  signal.symbol, ", ", static_cast<uint64_t>(signal.type), ", ");
                        m_signals_generated.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            }
            else
            {
                // Queue empty
                /// @todo benchmark against busy-spinning
                std::this_thread::yield();
            }
        }
        LOG_INFO("Strategy thread exiting.");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Strategy thread crashed with exception: ", e.what());
        m_client_running.store(false, std::memory_order_release);
    }
    catch (...)
    {
        LOG_ERROR("Strategy thread crashed with unknown exception");
        m_client_running.store(false, std::memory_order_release);
    }
}

void Core::_execution_loop()
{
    try
    {
        LOG_INFO("Execution thread started (Core 2)");
        Signal signal;

        while (m_client_running.load(std::memory_order_acquire))
        {
            if (m_signal_queue.pop(signal))
            {
                Order order = _generate_order(signal);

                // Send order out
                LOG_INFO("Executing order: ", order.symbol, " ", (order.side == Order::Side::BUY ? "BUY" : "SELL"), " ",
                         order.quantity, " @ ", order.price);

                m_orders_sent.fetch_add(1, std::memory_order_relaxed);

                /// @todo send order to market through the gateway
                ///         tcp_client->send or something
            }
            else
            {
                std::this_thread::yield();
            }
        }
        LOG_INFO("Execution thread exiting.");
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Execution thread crashed with exception: ", e.what());
        m_client_running.store(false, std::memory_order_release);
    }
    catch (...)
    {
        LOG_ERROR("Execution thread crashed with unknown exception");
        m_client_running.store(false, std::memory_order_release);
    }
}

Core::Signal Core::_compute_signal(const MarketData &md)
{
    quick::utils::Timer timer{"_compute_signal()"};
    Signal signal;
    std::memcpy(signal.symbol, md.symbol, sizeof(signal.symbol));
    signal.timestamp_ns = md.timestamp_ns;

    // Spread check
    double spread = md.ask - md.bid;
    double mid = (md.ask + md.bid) / 2.0;
    LOG_DEBUG("ask, bid:", md.ask, ", ", md.bid);

    if (spread < 0.05) // Tight spread
    {
        if (mid < 190.0)
        {
            signal.type = Signal::Type::BUY;
            signal.price = md.ask;
            signal.quantity = 100;
        }
        else if (mid > 190.0005)
        {
            signal.type = Signal::Type::SELL;
            signal.price = md.bid;
            signal.quantity = 100;
        }
        else
        {
            signal.type = Signal::Type::HOLD;
            signal.price = -1;
        }
    }

    LOG_DEBUG("Signal: ", (signal.type == Signal::Type::BUY) ? "BUY " : "MAYBE-SELL ", signal.symbol, ", ",
              signal.timestamp_ns, ", ", signal.price, ", ", signal.quantity, ", ", mid, ", ");
    return signal;
}

Core::Order Core::_generate_order(const Core::Signal &signal)
{
    static std::atomic<uint64_t> s_order_id_counter{1};
    Order order;
    std::memcpy(order.symbol, signal.symbol, sizeof(order.symbol));
    order.side = (signal.type == Signal::Type::BUY) ? Order::Side::BUY : Order::Side::SELL;
    order.price = signal.price;
    order.quantity = signal.quantity;
    order.order_id = s_order_id_counter.fetch_add(1, std::memory_order_relaxed);

    auto now = std::chrono::steady_clock::now();
    order.timestamp_ns = now.time_since_epoch().count();
    return order;
}

void Core::_set_thread_affinity(std::thread::native_handle_type thread, int cpu_id)
{
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    int result = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (result != 0)
        LOG_WARN("Core failed to set thread affinity to CPU ID ", cpu_id, " ", "for thread ", thread);
    else
        LOG_INFO("Thread id ", thread, " pinned to CPU ", cpu_id);
#else
    LOG_WARN("Get on Linux so I can give your threads some affinity...")
#endif
}

void Core::stop_client()
{
    if (is_client_stopped())
    {
        LOG_WARN("Client already stopped.");
        return;
    }

    LOG_INFO("Stopping client...");
    m_client_stopping.store(true, std::memory_order_release);
    m_client_running.store(false, std::memory_order_release);

    // Request cooperative cancellation
    if (m_network_thread.joinable())
        m_network_thread.request_stop();
    if (m_strategy_thread.joinable())
        m_strategy_thread.request_stop();
    if (m_execution_thread.joinable())
        m_execution_thread.request_stop();

    m_client_stopped.store(true, std::memory_order_release);

    print_client_stats();
    LOG_INFO("Client stopped.");
    // jthread dtor handles joining
}

void Core::print_client_stats() const
{
    uint64_t ticks_received = p_market_feed ? p_market_feed->ticks_received() : 0;
    uint64_t queue_full_events = p_market_feed ? p_market_feed->queue_full_count() : 0;

    LOG_DEBUG("\n=== Core Statistics ===", "\n\tTicks received: ", ticks_received,
              "\n\tSignals generated: ", m_signals_generated.load(std::memory_order_relaxed),
              "\n\tOrders sent: ", m_orders_sent.load(std::memory_order_relaxed), "\n\tQueue full events: ",
              queue_full_events
              // Queue status
              ,
              "\n\tMarket data queue size: ", m_market_data_queue.size(),
              "\n\tSignal queue size: ", m_signal_queue.size(), "\n\tOrder queue size: ", m_order_queue.size());
}

structs::MarketData Core::generate_market_data(std::string symbol)
{
    static std::atomic<uint64_t> tick_counter{0};
    static std::mt19937_64 rng{std::random_device{}()};
    static std::uniform_real_distribution<double> price_dist{189.95, 190.05};
    static std::uniform_real_distribution<double> spread_dist{0.01, 0.05};

    const uint64_t tick_id = tick_counter.fetch_add(1, std::memory_order_relaxed);
    const double bid = price_dist(rng);
    const double ask = bid + spread_dist(rng);

    MarketData md{};
    md.seq_num = tick_id;
    std::strncpy(md.symbol, symbol.c_str(), sizeof(md.symbol) - 1);
    md.symbol[sizeof(md.symbol) - 1] = '\0'; // Ensure null termination
    md.bid = bid;
    md.ask = ask;
    md.timestamp_ns = tick_id;

    return md;
}

auto Core::work_server() -> std::expected<void, CoreError>
{
    quick::utils::Timer timer{"Core::work_server()"};
    using namespace std::chrono_literals;

    if (!is_server_initialized()) [[unlikely]]
    {
        auto result = initialize_server();
        if (!result.has_value())
        {
            LOG_ERROR("Core failed to initialize server.");
            return std::unexpected(result.error());
        }
    }

    // Invariant check: p_tcp_server must exist after initialization
    if (!p_tcp_server) [[unlikely]]
    {
        throw fiah::error::CoreException("FATAL: p_tcp_server is null after initialization", CoreError::INVALID_STATE);
    }

    LOG_INFO("Server listening on port ", p_config->get_market_port());
    LOG_INFO("Accepting client connections...");

    std::uint64_t i{0};
    do
    {
        // blocks while waiting for clients!
        auto client_socket = p_tcp_server->accept_client();
        if (!client_socket.has_value())
        {
            LOG_ERROR("Could not accept client - accept_client() failed");
            continue;
        }
        LOG_INFO("Got client with fd: ", client_socket.value());

        // Send market data continuously to this client until they disconnect
        for (;;)
        {
            quick::utils::Timer timer{"Core::work_server::LOOP"};
            if (client_socket.value() < 1)
            {
                LOG_WARN("Invalid client socket. Breaking send loop.");
                break;
            }

            auto tick = generate_market_data("AAPL");

            auto send_result = p_tcp_server->send(client_socket.value(), std::addressof(tick), sizeof(tick));
            if (!send_result.has_value())
            {
                LOG_INFO("Client fd: ", client_socket.value(), " disconnected. Waiting for next client...");
                break; // Client disconnected, go back to accept loop
            }

            LOG_DEBUG("Sent tick ", i, " to client fd: ", client_socket.value());
            std::this_thread::sleep_for(1s);
        }
    } while (true);

    // Unreachable in normal operation, but needed for signature
    return {};
}

} // End namespace fiah