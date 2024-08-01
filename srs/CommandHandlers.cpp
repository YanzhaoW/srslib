#include <array>
#include <numeric>
#include <ranges>
#include <srs/CommandHandlers.hpp>
#include <srs/Control.hpp>
#include <vector>

namespace srs
{
    // TODO: why?
    constexpr auto magic_data_size = 16;

    using EntryType = uint32_t;

    // struct Address
    // {
    //     constexpr Address() = default;
    //     // NOLINTNEXTLINE(google-explicit-constructor, hicpp-explicit-conversions)
    //     constexpr Address(uint8_t val)
    //         : is_valid{ true }
    //         , value{ val }
    //     {
    //     }

    //     bool is_valid = false;
    //     uint8_t value = 0;
    // };

    // template <MessageMode mode, Address address = {}>
    // class SimpleHandler
    // {
    //   public:
    //     explicit SimpleHandler(const std::array<EntryType, 3>& data) {}
    //     void operator()(Control& control)
    //     {
    //         if constexpr (address.is_valid)
    //         {
    //             control.register_command<mode>(data_, address.value);
    //         }
    //         else
    //         {
    //             control.register_command<mode>(data_);
    //         }
    //     }

    //   private:
    //     std::array<EntryType, 3> data_{};
    // };

    // template <Address address = {}>
    // using WriteHandler = SimpleHandler<MessageMode::write, address>;

    // template <Address address = {}>
    // using ReadHandler = SimpleHandler<MessageMode::write, address>;
    inline auto make_reserved_vector(int size)
    {
        auto data = std::vector<EntryType>{};
        data.reserve(size);
        return data;
    }

    void system_register_handler(Control& control)
    {
        auto data = std::array<EntryType, magic_data_size + 1>{};
        data.front() = 0;
        std::iota(data.begin() + 1, data.end(), 0);

        control.register_command<MessageMode::read>(data);
    }

    void acq_on_handler(Control& control)
    {
        const auto data = std::array<EntryType, 3>{ 0, 15, 1 };
        control.register_command<MessageMode::write>(data);
    }

    void acq_off_handler(Control& control)
    {
        const auto data = std::array<EntryType, 3>{ 0, 15, 0 };
        control.register_command<MessageMode::write>(data);
    }

    void reset_handler(Control& control)
    {
        const auto data = std::array<EntryType, 3>{ 0, 0xffffffff, 0xffff0001 };
        control.register_command<MessageMode::write>(data, 0);
    }

    void powercycle_hybrids_handler(Control& control)
    {
        const auto data = std::array<EntryType, 3>{ 0, 0, 0x37f };
        control.register_command<MessageMode::write>(data, I2C_ADDRESS);
    }

    void set_mask_handler(Control& control)
    {
        const auto data = std::array<EntryType, 3>{ 0, 8, control.get_channel_address() };
        control.register_command<MessageMode::write>(data);
    }

    void link_status_handler(Control& control)
    {
        const auto data = std::array<EntryType, 2>{ 0, 16 };
        control.register_command<MessageMode::write>(data);
    }

    const CommandsHandlerMap commands_handler_map{ { Commands::system_registers, system_register_handler },
                                                   { Commands::set_mask, set_mask_handler },
                                                   { Commands::acq_on, acq_on_handler },
                                                   { Commands::acq_off, acq_off_handler },
                                                   { Commands::reset, reset_handler },
                                                   { Commands::link_status, link_status_handler },
                                                   { Commands::powercycle_hybrids, powercycle_hybrids_handler } };

} // namespace srs
