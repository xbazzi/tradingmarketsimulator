#pragma once

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>

#include "fiah/io/Config.hh"

namespace mms
{

/// @brief Utility class for managing test configurations
class TestConfigBuilder
{
  private:
    std::string m_market_ip{"127.0.0.1"};
    std::uint16_t m_market_port{1337};
    std::filesystem::path m_config_path;
    bool m_use_temp_file{false};

  public:
    TestConfigBuilder() = default;

    /// @brief Set custom market IP
    TestConfigBuilder &with_market_ip(const std::string &ip)
    {
        m_market_ip = ip;
        return *this;
    }

    /// @brief Set custom market port
    TestConfigBuilder &with_market_port(std::uint16_t port)
    {
        m_market_port = port;
        return *this;
    }

    /// @brief Use existing config file
    TestConfigBuilder &with_config_path(const std::filesystem::path &path)
    {
        m_config_path = path;
        m_use_temp_file = false;
        return *this;
    }

    /// @brief Create a temporary config file for testing
    TestConfigBuilder &with_temp_config()
    {
        m_use_temp_file = true;
        return *this;
    }

    /// @brief Build and return the config object
    fiah::Config build()
    {
        if (m_use_temp_file)
        {
            m_config_path = create_temp_config_file();
        }
        else if (m_config_path.empty())
        {
            m_config_path = "../config.toml";
        }

        fiah::Config config(m_config_path);
        config.parse_config();
        return config;
    }

  private:
    std::filesystem::path create_temp_config_file()
    {
        auto temp_path = std::filesystem::temp_directory_path() / "mms_test_config.toml";

        std::ofstream file(temp_path);
        file << "title = \"HFT Config\"\n\n";

        // Minimal required sections for Config::parse_config()
        file << "[logging]\n";
        file << "enabled = true\n";
        file << "level = \"INFO\"\n";
        file << "log_to_file = false\n";
        file << "log_path = \"/tmp/mms.log\"\n";
        file << "max_file_size_mb = 10\n";
        file << "rotation_count = 3\n\n";

        file << "[trading]\n";
        file << "enabled = false\n";
        file << "strategy = \"default\"\n";
        file << "max_position_size = 0\n";
        file << "max_order_size = 0\n";
        file << "risk_limit_usd = 0.0\n";
        file << "tick_size = 0.01\n";
        file << "latency_threshold_us = 1000\n\n";

        file << "[network]\n";
        file << "tcp_buffer_size = 65536\n";
        file << "udp_buffer_size = 65536\n";
        file << "socket_timeout_ms = 1000\n";
        file << "keepalive_interval_s = 60\n";
        file << "max_connections = 100\n";
        file << "enable_nagle = false\n\n";

        file << "[system]\n";
        file << "num_threads = 1\n";
        file << "cpu_affinity = [0]\n";
        file << "use_huge_pages = false\n";
        file << "priority = \"normal\"\n";
        file << "watchdog_timeout_s = 30\n\n";

        file << "[servers]\n";
        file << "[servers.market]\n";
        file << "ip = \"" << m_market_ip << "\"\n";
        file << "port = " << m_market_port << "\n";
        file << "protocol = \"tcp\"\n";
        file << "[servers.beta]\n";
        file << "ip = \"127.0.0.1\"\n";
        file << "port = 1338\n";
        file << "protocol = \"tcp\"\n";
        file << "[servers.risk]\n";
        file << "ip = \"127.0.0.1\"\n";
        file << "port = 1339\n";
        file << "protocol = \"tcp\"\n";
        file << "\n[database]\n";
        file << "enabled = false\n";
        file << "host = \"127.0.0.1\"\n";
        file << "port = 5432\n";
        file << "name = \"mms_db\"\n";
        file << "user = \"mms\"\n";
        file << "password = \"secret\"\n";
        file << "connection_pool_size = 1\n";
        file.close();

        return temp_path;
    }
};

/// @brief Helper to create default test config
inline fiah::Config create_default_test_config()
{
    return TestConfigBuilder().with_config_path("../etc/config.toml").build();
}

/// @brief Helper to create config with custom port
inline fiah::Config create_test_config_with_port(std::uint16_t port)
{
    return TestConfigBuilder().with_market_port(port).with_temp_config().build();
}

/// @brief RAII wrapper for cleaning up temp files
class TempFileGuard
{
  private:
    std::filesystem::path m_path;
    bool m_should_delete;

  public:
    explicit TempFileGuard(std::filesystem::path path, bool should_delete = true)
        : m_path(std::move(path)), m_should_delete(should_delete)
    {
    }

    ~TempFileGuard()
    {
        if (m_should_delete && std::filesystem::exists(m_path))
        {
            std::filesystem::remove(m_path);
        }
    }

    TempFileGuard(const TempFileGuard &) = delete;
    TempFileGuard &operator=(const TempFileGuard &) = delete;
    TempFileGuard(TempFileGuard &&) = default;
    TempFileGuard &operator=(TempFileGuard &&) = default;

    const std::filesystem::path &path() const
    {
        return m_path;
    }
};

/// @brief Test helper to capture log output (placeholder for future implementation)
class LogCapture
{
  public:
    LogCapture() = default;

    // Future: Capture log output for verification
    std::string get_logs() const
    {
        return "";
    }
    void clear()
    {
    }
};

} // namespace mms

// Source - https://stackoverflow.com/a
// Posted by Mark Lakata, modified by community. See post 'Timeline' for change history
// Retrieved 2025-11-16, License - CC BY-SA 4.0

// C++ stream interface
class TestCout : public std::stringstream
{
  public:
    ~TestCout()
    {
        std::cout << "\u001b[32m[          ] \u001b[33m" << str() << "\u001b[0m" << std::flush;
    }
};

#define TEST_COUT TestCout()
