#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

namespace srs
{
    using BufferType = std::vector<uint8_t>;
    using EntryType = uint32_t;

    constexpr auto WRITE_COMMAND_BITS = uint8_t{ 0xaa };
    constexpr auto DEFAULT_TYPE_BITS = uint8_t{ 0xaa };
    constexpr auto COMMAND_LENGTH_BITS = uint16_t{ 0xffff };
    constexpr auto ZERO_UINT16_PADDING = uint16_t{};
    constexpr auto READ_MSG_BUFFER_SIZE = 1000;

    constexpr auto DEFAULT_CMD_LENGTH = uint16_t{ 0xffff };
    constexpr auto CMD_TYPE = uint8_t{ 0xaa };
    constexpr auto WRITE_CMD = uint8_t{ 0xaa };
    constexpr auto READ_CMD = uint8_t{ 0xbb };
    constexpr auto I2C_ADDRESS = uint16_t{ 0x0042 };  /* device address = 0x21 */
    constexpr auto NULL_ADDRESS = uint16_t{ 0x000f }; /* device address = 0x21 */

    constexpr auto INIT_COUNT_VALUE = uint32_t{ 0x80000000 };
    constexpr auto DEFAULT_STATUS_WAITING_TIME_SECONDS = std::chrono::seconds{ 4 };
} // namespace srs
