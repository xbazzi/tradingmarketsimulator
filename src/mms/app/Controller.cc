// C++ Includes
#include <chrono>
#include <expected>

// MarketMakerSimulator Includes
#include "mms/app/Controller.hh"
#include "mms/app/Core.hh"
#include "fiah/handle/UniquePtr.hh"
#include "fiah/io/Config.hh"
#include "fiah/utils/Logger.hh"
#include "fiah/utils/Timer.hh"

namespace mms
{

Controller::Controller(fiah::Config &&config) noexcept(noexcept(CoreUniquePtr()))
    : p_core{fiah::make_unique<Core>(std::move(config))}
{
}

bool Controller::init_client() noexcept
{
    auto result = p_core->initialize_client();
    if (!result.has_value())
    {
        /// @todo print error as string in the log
        LOG_ERROR("Failed to init client in Controller: <ErrorFromCore>");
        return 1;
    }
    LOG_INFO("Controller successfully initialized the client.");
    return 0;
}

bool Controller::init_server() noexcept
{
    auto result = p_core->initialize_server();
    if (!result.has_value())
    {
        /// @todo print error as string in the log
        LOG_ERROR("Failed to init server in Controller: <ErrorFromCore>");
        return 1;
    }
    LOG_INFO("Controller successfully initialized the server.");
    return 0;
}

bool Controller::start_server() noexcept
{
    try
    {
        if (!p_core->is_server_initialized())
        {
            if (init_server() != 0)
            {
                LOG_ERROR("Failed to initialize server. Aborting start!");
                return 1;
            }
        }
        p_core->work_server();
        return 0;
    }
    catch (CoreException &e)
    {
        LOG_ERROR("Controller failed to start server (mms::CoreException): ", e.what());
        return 1;
    }
    catch (std::exception &e)
    {
        LOG_ERROR("Controller failed to start server (std::exception): ", e.what());
        return 1;
    }
}

bool Controller::start_client() noexcept
{
    try
    {
        using namespace std::chrono_literals;
        if (init_client() != 0)
        {
            LOG_ERROR("Failed to initialize client. Aborting start!");
            return 1;
        }
        auto work_result = p_core->work_client();
        if (!work_result.has_value())
        {
            LOG_ERROR("Client work failed");
            return 1;
        }

        // Let client run for a significant amount of time to process many
        // ticks
        /// @todo control with signal handlers or events instead of time
        LOG_INFO("Client running. Press Ctrl+C to stop...");
        std::this_thread::sleep_for(600s); // Run for 10 minutes

        p_core->stop_client();
        return 0;
    }
    catch (CoreException &e)
    {
        LOG_ERROR("Controller failed to start client (mms::CoreException): ", e.what());
        return 1;
    }
    catch (std::exception &e)
    {
        LOG_ERROR("Controller failed to start client (std::exception): ", e.what());
        return 1;
    }
}

} // End namespace mms