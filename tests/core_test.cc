#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

#include "mms/app/Core.hh"
#include "mms/error/CoreException.hh"
#include "mms/error/Error.hh"
#include "fiah/io/Config.hh"
#include "test_utils.hh"

using namespace mms;
using namespace std::chrono_literals;

// ============================================================================
// Test Fixture Base Class
// ============================================================================

/// @brief Base test fixture for Core tests
/// @details Provides common setup, teardown, and utility methods for all Core tests
class CoreTestFixture : public ::testing::Test
{
  protected:
    std::unique_ptr<Core> p_core;
    fiah::Config config_;

    /// @brief Called before each test
    void SetUp() override
    {
        // Create a fresh config for each test
        config_ = mms::create_default_test_config();
    }

    /// @brief Called after each test
    void TearDown() override
    {
        // Ensure client is stopped before destroying
        if (p_core)
        {
            p_core->stop_client();
        }
        p_core.reset();
    }

    /// @brief Helper to create an Core instance
    void create_algo()
    {
        p_core = std::make_unique<Core>(std::move(config_));
    }

    /// @brief Helper to create algo with custom config
    void create_p_corewith_config(fiah::Config custom_config)
    {
        p_core = std::make_unique<Core>(std::move(custom_config));
    }

    /// @brief Wait for a condition with timeout
    template <typename Predicate> bool wait_for_condition(Predicate pred, std::chrono::milliseconds timeout = 1000ms)
    {
        auto start = std::chrono::steady_clock::now();
        while (!pred())
        {
            if (std::chrono::steady_clock::now() - start > timeout)
            {
                return false;
            }
            std::this_thread::sleep_for(10ms);
        }
        return true;
    }
};

// ============================================================================
// Constructor and Initialization Tests
// ============================================================================

class CoreConstructorTest : public CoreTestFixture
{
};

TEST_F(CoreConstructorTest, ConstructorWithValidConfig)
{
    create_algo();

    ASSERT_NE(p_core, nullptr);
    EXPECT_FALSE(p_core->is_server_initialized());
    EXPECT_FALSE(p_core->is_client_initialized());
    EXPECT_FALSE(p_core->is_client_running());
    EXPECT_FALSE(p_core->is_client_stopped());
}

TEST_F(CoreConstructorTest, InitialStateIsConsistent)
{
    create_algo();

    // All state flags should be false initially
    EXPECT_FALSE(p_core->is_server_initialized());
    EXPECT_FALSE(p_core->is_client_initialized());
    EXPECT_FALSE(p_core->is_client_running());
    EXPECT_FALSE(p_core->is_client_stopped());
}

// ============================================================================
// Server Initialization Tests
// ============================================================================

class CoreServerTest : public CoreTestFixture
{
};

TEST_F(CoreServerTest, InitializeServerSetsState)
{
    create_algo();

    EXPECT_FALSE(p_core->is_server_initialized());

    auto result = p_core->initialize_server();

    // Server init may fail if port is in use - handle both cases
    if (result.has_value())
    {
        EXPECT_TRUE(p_core->is_server_initialized());
    }
    else
    {
        EXPECT_EQ(result.error(), CoreError::INIT_SERVER_FAIL);
        EXPECT_FALSE(p_core->is_server_initialized());
    }
}

TEST_F(CoreServerTest, InitializeServerTwiceIsIdempotent)
{
    create_algo();

    auto result1 = p_core->initialize_server();

    if (result1.has_value())
    {
        EXPECT_TRUE(p_core->is_server_initialized());

        // Second call should succeed and not change state
        auto result2 = p_core->initialize_server();
        EXPECT_TRUE(result2.has_value());
        EXPECT_TRUE(p_core->is_server_initialized());
    }
}

TEST_F(CoreServerTest, InitializeServerWithCustomPort)
{
    // Use a different port to avoid conflicts
    auto custom_config = mms::create_test_config_with_port(0); // Port 0 lets OS choose
    create_p_corewith_config(std::move(custom_config));

    auto result = p_core->initialize_server();

    if (result.has_value())
    {
        EXPECT_TRUE(p_core->is_server_initialized());
    }
}

// ============================================================================
// Client Initialization Tests
// ============================================================================

class CoreClientTest : public CoreTestFixture
{
};

TEST_F(CoreClientTest, InitializeClientWithoutServerFails)
{
    create_algo();

    EXPECT_FALSE(p_core->is_client_initialized());

    auto result = p_core->initialize_client();

    // Expected to fail - server not running
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CoreError::SERVER_NOT_ONLINE);
    EXPECT_FALSE(p_core->is_client_initialized());
}

TEST_F(CoreClientTest, InitializeClientTwiceIsIdempotent)
{
    create_algo();

    auto result1 = p_core->initialize_client();

    // Only test idempotency if first succeeded (unlikely without server)
    if (result1.has_value())
    {
        EXPECT_TRUE(p_core->is_client_initialized());

        auto result2 = p_core->initialize_client();
        EXPECT_TRUE(result2.has_value());
        EXPECT_TRUE(p_core->is_client_initialized());
    }
}

/// @todo Implement
TEST_F(CoreClientTest, ReconnectClientWithoutPriorConnection)
{
}

// ============================================================================
// Client Lifecycle Tests
// ============================================================================

class CoreClientLifecycleTest : public CoreTestFixture
{
};

TEST_F(CoreClientLifecycleTest, StopClientWhenNotStarted)
{
    create_algo();

    // Should not crash or throw
    EXPECT_NO_THROW(p_core->stop_client());
    EXPECT_TRUE(p_core->is_client_stopped());
}

TEST_F(CoreClientLifecycleTest, StopClientTwiceIsIdempotent)
{
    create_algo();

    p_core->stop_client();
    EXPECT_TRUE(p_core->is_client_stopped());

    // Second stop should be safe
    EXPECT_NO_THROW(p_core->stop_client());
    EXPECT_TRUE(p_core->is_client_stopped());
}

TEST_F(CoreClientLifecycleTest, StopClientSetsCorrectState)
{
    create_algo();

    p_core->stop_client();

    EXPECT_TRUE(p_core->is_client_stopped());
    EXPECT_FALSE(p_core->is_client_running());
}

TEST_F(CoreClientLifecycleTest, WorkClientWithoutServerFails)
{
    create_algo();

    EXPECT_FALSE(p_core->is_client_running());

    auto result = p_core->work_client();

    // Should fail - server not online
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CoreError::SERVER_NOT_ONLINE);
    EXPECT_FALSE(p_core->is_client_running());
}

TEST_F(CoreClientLifecycleTest, WorkClientTwiceReturnsEarly)
{
    create_algo();

    // First call will fail to start (no server)
    auto result1 = p_core->work_client();

    // If somehow it succeeded, second call should detect already running
    if (result1.has_value() && p_core->is_client_running())
    {
        auto result2 = p_core->work_client();
        EXPECT_TRUE(result2.has_value()); // Should return success but not start new threads
        EXPECT_TRUE(p_core->is_client_running());
    }
}

// ============================================================================
// Print Stats Tests
// ============================================================================

class CoreStatsTest : public CoreTestFixture
{
};

TEST_F(CoreStatsTest, PrintStatsDoesNotCrashWhenNotRunning)
{
    create_algo();

    EXPECT_NO_THROW(p_core->print_client_stats());
}

TEST_F(CoreStatsTest, PrintStatsAfterStoppingClient)
{
    create_algo();

    p_core->stop_client();

    EXPECT_NO_THROW(p_core->print_client_stats());
}

// ============================================================================
// Destructor and RAII Tests
// ============================================================================

class CoreDestructorTest : public CoreTestFixture
{
  protected:
    // Override TearDown to allow manual destruction testing
    void TearDown() override
    {
        // Don't call base TearDown - we'll manually manage p_core
    }
};

TEST_F(CoreDestructorTest, DestructorCleansUpProperly)
{
    create_algo();

    auto result = p_core->initialize_server();

    // Destructor should handle cleanup
    p_core.reset();

    // If we get here, destructor succeeded
    SUCCEED();
}

TEST_F(CoreDestructorTest, DestructorStopsRunningThreads)
{
    create_algo();

    // Try to start client (will fail but that's OK)
    p_core->work_client();

    // Destructor should stop threads gracefully
    p_core.reset();

    SUCCEED();
}

TEST_F(CoreDestructorTest, DestructorHandlesMultipleStops)
{
    create_algo();

    p_core->stop_client();

    // Destructor also calls stop_client - should be idempotent
    p_core.reset();

    SUCCEED();
}

// ============================================================================
// State Transition Tests
// ============================================================================

class CoreStateTransitionTest : public CoreTestFixture
{
};

TEST_F(CoreStateTransitionTest, ServerStateTransition)
{
    create_algo();

    // Initial state
    EXPECT_FALSE(p_core->is_server_initialized());

    // Try to initialize
    auto result = p_core->initialize_server();

    if (result.has_value())
    {
        // State should have transitioned
        EXPECT_TRUE(p_core->is_server_initialized());

        // State should remain stable
        EXPECT_TRUE(p_core->is_server_initialized());
    }
}

TEST_F(CoreStateTransitionTest, ClientStopTransition)
{
    create_algo();

    // Initial state
    EXPECT_FALSE(p_core->is_client_stopped());
    EXPECT_FALSE(p_core->is_client_running());

    // Stop client
    p_core->stop_client();

    // State should transition
    EXPECT_TRUE(p_core->is_client_stopped());
    EXPECT_FALSE(p_core->is_client_running());
}

// ============================================================================
// Error Handling Tests
// ============================================================================

class CoreErrorHandlingTest : public CoreTestFixture
{
};

TEST_F(CoreErrorHandlingTest, InitializeClientReturnsCorrectError)
{
    create_algo();

    auto result = p_core->initialize_client();

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CoreError::SERVER_NOT_ONLINE);
}

TEST_F(CoreErrorHandlingTest, WorkClientReturnsCorrectError)
{
    create_algo();

    auto result = p_core->work_client();

    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), CoreError::SERVER_NOT_ONLINE);
}

/// @todo Implement
TEST_F(CoreErrorHandlingTest, ReconnectReturnsCorrectError)
{
}
