#include "Control.hpp"
#include "utils/Serializer.hpp"
#include <Connection.hpp>
#include <fmt/ranges.h>
#include <ranges>
#include <string_view>

namespace srs
{
    namespace this_coro = asio::this_coro;

    void Control::set_remote_endpoint(std::string_view remote_ip, int port_number)
    {
        auto resolver = udp::resolver{ *io_context_ };
        fmt::print("Connecting to socket with ip: {} and port: {}\n", remote_ip, port_number);
        auto udp_endpoints = resolver.resolve(udp::v4(), remote_ip, fmt::format("{}", port_number));
        remote_endpoint_ = *udp_endpoints.begin();
    }

    // void Control::set_host_port_number(int port_number)
    // {
    //     listen_socket_.close();
    //     listen_socket_.open(udp::v4());
    //     listen_socket_.bind(udp::endpoint(udp::v4(), port_number));
    // }

    struct CommandAcqOn
    {
        uint16_t zero_0 = 0U;
        uint16_t address = 0x000f;
        uint8_t cmd = 0xaa;
        uint8_t type = 0xaa;
        uint16_t cmd_length = 0xffff;
        std::array<uint32_t, 3> data{ 0U, 15U, 1U };
    };

    struct CommandAcqOff
    {
        uint16_t zero_0 = 0U;
        uint16_t address = 0x000f;
        uint8_t cmd = 0xaa;
        uint8_t type = 0xaa;
        uint16_t cmd_length = 0xffff;
        std::array<uint32_t, 3> data{ 0U, 15U, 0U };
    };

    void Control::configure_fec() {}

    struct CommandReset
    {
        uint16_t zero_0 = 0U;
        uint16_t address = 0U;
        uint8_t cmd = 0xaa;
        uint8_t type = 0xaa;
        uint16_t cmd_length = 0xffff;
        std::array<uint32_t, 3> data{ 0, 0xffffffff, 0xffff0001 };
    };

    void Control::encode_write_message(const std::vector<EntryType>& data, uint16_t address)
    {
        uint32_t counter = 0x80000000;
        serialize(output_buffer_,
                  counter,
                  ZERO_UINT16_PADDING,
                  address,
                  WRITE_COMMAND_BITS,
                  DEFAULT_TYPE_BITS,
                  COMMAND_LENGTH_BITS);
        for (const auto entry : data)
        {
            serialize(output_buffer_, entry);
        }
    }

    void Control::switch_on()
    {
        auto socket = std::make_shared<udp::socket>(*io_context_, udp::endpoint{ udp::v4(), default_port_number_ });
        auto connection_info = ConnectionInfo{ io_context_ };
        connection_info.socket = std::move(socket);
        connection_info.endpoint = &remote_endpoint_;
        auto connection = std::make_shared<Starter>(std::move(connection_info));
        connection->acq_on();
    }

} // namespace srs
