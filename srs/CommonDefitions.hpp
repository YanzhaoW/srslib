#pragma once

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
} // namespace srs
