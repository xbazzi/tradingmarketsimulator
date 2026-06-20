#pragma once

#include <cstdint>

namespace mms
{
enum class CoreError : std::uint8_t
{
    INIT_CLIENT_FAIL,
    SERVER_NOT_ONLINE,
    INIT_SERVER_FAIL,
    INVALID_STATE
};

enum class TomlParserError : std::uint8_t
{
    FILE_NOT_FOUND,
    PARSE_ERROR,
    KEY_NOT_FOUND,
    TYPE_MISMATCH,
    INVALID_TOML
};

enum class Error : std::uint8_t
{
    USER_ERROR,
    CONFIG_ERROR,
    UNKNOWN_ERROR,
    INIT_ERROR,
    CONTROLLER_ERROR,
    CORE_ERORR,
    THREAD_ERROR
};

enum class TcpError : std::uint8_t
{
    BAD_SOCKET,
    BIND_FAIL,
    LISTEN_FAIL,
    CONNECT_FAIL,
    ACCEPT_FAIL,
    SEND_FAIL,
    RECV_FAIL,
    INVALID_IP
};
} // End namespace mms