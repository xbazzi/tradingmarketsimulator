#ifndef COREEXCEPTION_HH
#define COREEXCEPTION_HH

// C++ Includes
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

// MarketMakerSimulator includes
#include "mms/error/Error.hh"
// #include "fiah/io/TcpError.hh"

namespace mms
{

using ErrorVariant = std::variant<std::monostate, TcpError, CoreError, Error>;

class CoreException : public std::runtime_error
{
  private:
    ErrorVariant m_error_variant{};

  public:
    explicit CoreException(const std::string &message) : std::runtime_error{message}, m_error_variant{std::monostate{}}
    {
    }

    template <typename EnumType>
        requires std::is_enum_v<EnumType>
    CoreException(const std::string &message, EnumType error)
        : std::runtime_error(format_error(message, error)), m_error_variant(error)
    {
    }

    const ErrorVariant &get_error() const noexcept
    {
        return m_error_variant;
    }

  private:
    template <typename EnumType> static std::string format_error(const std::string &message, EnumType error)
    {
        std::ostringstream oss;
        oss << message << " (" << enum_to_string(error) << ")";
        return oss.str();
    }

    static std::string enum_to_string(TcpError e)
    {
        switch (e)
        {
        case TcpError::BAD_SOCKET:
            return "TcpError::BAD_SOCKET";
        case TcpError::BIND_FAIL:
            return "TcpError::BIND_FAIL";
        case TcpError::LISTEN_FAIL:
            return "TcpError::LISTEN_FAIL";
        case TcpError::CONNECT_FAIL:
            return "TcpError::CONNECT_FAIL";
        case TcpError::ACCEPT_FAIL:
            return "TcpError::ACCEPT_FAIL";
        case TcpError::SEND_FAIL:
            return "TcpError::SEND_FAIL";
        case TcpError::RECV_FAIL:
            return "TcpError::RECV_FAIL";
        case TcpError::INVALID_IP:
            return "TcpError::INVALID_IP";
        }
        return "TcpError::UNKNOWN";
    }

    static std::string enum_to_string(CoreError e)
    {
        switch (e)
        {
        case CoreError::INIT_CLIENT_FAIL:
            return "CoreError::INIT_CLIENT_FAIL";
        case CoreError::SERVER_NOT_ONLINE:
            return "CoreError::SERVER_NOT_ONLINE";
        case CoreError::INIT_SERVER_FAIL:
            return "CoreError::INIT_SERVER_FAIL";
        case CoreError::INVALID_STATE:
            return "CoreError::INIT_SERVER_FAIL";
        }
        return "CoreError::UNKNOWN";
    }

    static std::string enum_to_string(Error e)
    {
        switch (e)
        {
        case Error::USER_ERROR:
            return "Error::USER_ERROR";
        case Error::CONFIG_ERROR:
            return "Error::CONFIG_ERROR";
        case Error::UNKNOWN_ERROR:
            return "Error::UNKNOWN_ERROR";
        case Error::INIT_ERROR:
            return "Error::INIT_ERROR";
        case Error::CONTROLLER_ERROR:
            return "Error::CONTROLLER_ERROR";
        case Error::CORE_ERORR:
            return "Error::CORE_ERROR";
        case Error::THREAD_ERROR:
            return "Error::THREAD_ERROR";
        }
        return "Error::UNKNOWN";
    }
};

} // namespace mms

#endif // CORE_HH
