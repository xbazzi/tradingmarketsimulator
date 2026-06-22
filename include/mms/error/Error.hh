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
    THREAD_ERROR,
    DeserializeError
};

} // End namespace mms