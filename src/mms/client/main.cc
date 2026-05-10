// C++ Includes
#include <cassert>
#include <csignal>
#include <cstdint>
#include <cxxabi.h>
#include <filesystem>
#include <iostream>
#include <sstream>

// MarketMakerSimulator Includes
#include "mms/app/Controller.hh"
#include "mms/structs/Structs.hh"
#include "mms/utils/TomlParser.hh"
#include "fiah/io/Config.hh"
#include "fiah/utils/SimpleLogger.hh"
#include "fiah/utils/Timer.hh"

void print_help()
{
    std::ostringstream ss;
    ss << "Usage: " << '\n' << "\tclient <config_file_path>" << '\n' << "\tEx: client etc/config.toml" << '\n';
    std::cout << ss.str() << std::endl;
}

static std::atomic<bool> g_shutdown;

void signal_handler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\nShutdown signal received..." << std::endl;
        g_shutdown.store(true, std::memory_order_seq_cst);
    }
}

/// @brief Look at this dummy
struct DummyStructForMainLoggerTag
{
};

int main(int argc, char *argv[])
{
#ifdef _LIBCPP_VERSION
    std::cout << "Using libc++ " << _LIBCPP_VERSION << '\n';
#elif defined(__GLIBCXX__)
    std::cout << "Using libstdc++ " << __GLIBCXX__ << '\n';
#else
    std::cout << "Unknown standard library\n";
#endif
    // initialize the simple global logger for this binary
    fiah::init("main");
    LOG_DEBUG_S("Client entrypoint (client_main.cc) started.");

    if (argc < 2)
    {
        print_help();
        return 1;
    }

    const auto &path = std::filesystem::path(argv[1]);
    if (!std::filesystem::exists(path))
    {
        std::cout << "Path doesn't exist: " << path << '\n';
        return 1;
    }

    fiah::Config config{path};
    if (!config.parse_config())
    {
        std::cout << "Unable to parse config" << '\n';
        return 1;
    }

    /// Create this mf on the stack keep it a stack💯
    mms::Controller ctlr(std::move(config));

    /// Start some work around here
    if (auto rc = ctlr.start_client(); rc == 1)
    {
        LOG_ERROR_S("Client entrypoint exited with status ", rc);
        return rc;
    }
    LOG_INFO_S("Client entrypoint exited with status ", 0);
    LOG_INFO_S("Market IP: ", config.get_market_ip());
    LOG_INFO_S("Market Port: ", config.get_market_port());
    return 0;
}