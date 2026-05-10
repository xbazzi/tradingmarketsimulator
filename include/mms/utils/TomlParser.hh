/**
 * @file TomlParser.hh
 * @author Xander Bazzi (codemaster@xbazzi.com)
 * @brief
 * @version 0.1
 * @date 2025-11-08
 *
 * @copyright Copyright (c) 2025
 *
 */

#pragma once
// C++ Includes
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// QUICK Includes
#include "fiah/utils/Logger.hh"

// Third Party Includes

// MarketMakerSimulator Includes
#include "mms/error/Error.hh"

namespace mms
{
/**
 * @brief Simple TOML parser for configuration files
 *
 */
class TomlParser
{
  private:
    // std::ifstream m_ifs;
    std::filesystem::path m_filepath;
    std::vector<std::string> m_sections;
    static inline fiah::Logger<TomlParser> &m_logger{
        fiah::Logger<TomlParser>::get_instance("TomlParser")};

    // Map of section -> (key -> value)
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_keys;

  public:
    TomlParser() noexcept;
    TomlParser(const std::filesystem::path &path) noexcept;

    ~TomlParser() = default;

    /** @brief Validate the TOML file
     *  @param file The input file stream
     *  @return true if valid, false otherwise
     */
    std::expected<bool, TomlParserError> is_valid(std::ifstream &file) noexcept;

    std::expected<bool, TomlParserError> extract_keys(std::ifstream &file) noexcept;

    /** @brief Load a TOML file from disk
     *  @return true if successful, false otherwise
     */
    std::expected<bool, TomlParserError> load() noexcept;

    /** @brief Get a value from the TOML file as a string
     *  @param key The key to look up
     *  @return The value as a string
     */
    std::string get_string(const std::string &key) const;

    /** @brief Get a value from the TOML file as an integer
     *  @param key The key to look up
     *  @return The value as an integer
     */
    int get_int(const std::string &key) const;

    /** @brief Get a value from the TOML file as a double
     *  @param key The key to look up
     *  @return The value as a double
     */
    double get_double(const std::string &key) const;

    std::optional<std::string> get_value(const std::string &section, const std::string &key) const;
};
} // namespace mms