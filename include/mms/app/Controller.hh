#ifndef MMS_CONTROLLER_HH
#define MMS_CONTROLLER_HH
#endif

#include "mms/app/Core.hh"
#include "quick/handle/UniquePtr.hh"
#include "quick/io/Config.hh"
#include "quick/utils/Logger.hh"

namespace mms
{

/// @brief Initializes Core and orchestrates high-level work, such as
///        starting the server and client comms and strategies.
/// @attention Controller is a termination point, and as such it catches
///            all exceptions without throwing back to the calling method
///            (usually main).
class Controller
{
  public:
    Controller() noexcept = default;
    explicit Controller(quick::Config &&) noexcept(noexcept(CoreUniquePtr()));
    explicit Controller(const quick::Config &) = delete;
    bool start_server() noexcept;
    bool start_client() noexcept;
    bool init_client() noexcept;
    bool init_server() noexcept;

  private:
    using CoreUniquePtr = quick::handle::UniquePtr<Core>;
    CoreUniquePtr p_core{nullptr};
    static inline quick::utils::Logger<Controller> &m_logger{
        quick::utils::Logger<Controller>::get_instance("Controller")};
};

} // End namespace mms