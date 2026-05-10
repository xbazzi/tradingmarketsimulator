#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <print>
#include <ratio>
#include <sstream>
#include <string_view>

namespace mms
{
using namespace std::literals::chrono_literals;

class Timer
{
  private:
    using clock = std::chrono::steady_clock;
    using time_point = std::chrono::time_point<clock>;
    /// @brief
    std::string_view m_scope_name;
    time_point m_start_timepoint;

    [[nodiscard]] auto elapsed() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - m_start_timepoint);
    }

    void stop()
    {
        std::chrono::microseconds dur = elapsed();
        std::ostringstream oss;
        oss << "\033[35m"
            << "[TIMER] " << m_scope_name << " took ≈ " << dur / 1us << "us"
            << " ≈ " << std::chrono::duration_cast<std::chrono::seconds>(dur).count() << "s\n"
            << "\033[0m";
        std::cout << oss.str() << std::flush;
        /// @todo Switch to C++23 print
        // std::print("{}[Timer] {} took {}ns",
        //     std::chrono::utc_clock::now(),
        //     m_scope_name,
        //     dur.count());
    }

    void reset()
    {
        m_start_timepoint = clock::now();
    }

  public:
    Timer() noexcept : m_scope_name{"Unspecified"}, m_start_timepoint{clock::now()}
    {
    }
    Timer(const std::string_view scope_name) noexcept : m_scope_name{scope_name}, m_start_timepoint{clock::now()}
    {
    }
    Timer(const Timer &) = delete;
    Timer(Timer &&) = delete;
    Timer &operator=(const Timer &) = delete;
    Timer &operator=(Timer &&) = delete;

    ~Timer() noexcept
    {
        stop();
    }
};
} // End namespace mms