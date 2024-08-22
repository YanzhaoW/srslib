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
    constexpr auto I2C_ADDRESS = uint16_t{ 0x0042 }; /* device address = 0x21 */
    constexpr auto NULL_ADDRESS = uint16_t{ 0x000f }; /* device address = 0x21 */

    enum class Commands
    {
        acq_on,
        acq_off,
        configure_hybrid,
        link_status,
        powercycle_hybrids,
        read_adc,
        reset,
        send_config,
        set_mask,
        system_registers,
        trigger_acq_constants,
    };

    enum class MessageMode
    {
        read,
        write
    };

    struct DeviceIndex
    {
        uint8_t hybrid{};
        uint8_t vmm{};
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

    using CommandsHandlerMap = std::map<Commands, std::function<void(Control&, DeviceIndex)>>;
    extern const CommandsHandlerMap commands_handler_map;

} // namespace srs
