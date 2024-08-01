#pragma once

#include <cstdint>
#include <functional>
#include <map>

namespace srs
{

    constexpr auto DEFAULT_CMD_LENGTH = uint16_t{ 0xffff };
    constexpr auto CMD_TYPE = uint8_t{ 0xaa };
    constexpr auto WRITE_CMD = uint8_t{ 0xaa };
    constexpr auto READ_CMD = uint8_t{ 0xbb };
    constexpr auto I2C_ADDRESS = uint8_t{ 0x42 }; /* device address = 0x21 */

    enum class Commands
    {
        system_registers,
        reset,
        powercycle_hybrids,
        link_status,
        trigger_acq_constants,
        acq_on,
        acq_off,
        set_mask,
        configure_hybrid,
        send_config,
        read_adc,
    };

    enum class MessageMode
    {
        read,
        write
    };

    struct MessageHeader
    {
        constexpr explicit MessageHeader(uint8_t CMD)
            : cmd{ CMD }
        {
        }

        uint8_t cmd = 0;
        uint8_t type = CMD_TYPE;
        uint16_t cmd_length = DEFAULT_CMD_LENGTH;
    };

    class Control;

    using CommandsHandlerMap = std::map<Commands, std::function<void(Control&)>>;
    extern const CommandsHandlerMap commands_handler_map;

} // namespace srs
