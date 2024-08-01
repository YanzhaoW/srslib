#include "Control.hpp"
#include "utils/Serializer.hpp"
#include <fmt/ranges.h>
#include <string_view>

namespace srs
{
    void Control::set_remote_endpoint(std::string_view remote_ip, int port_number)
    {
        auto resolver = udp::resolver{ io_context_ };
        fmt::print("Connecting to socket with ip: {} and port: {}\n", remote_ip, port_number);
        auto udp_endpoints = resolver.resolve(udp::v4(), remote_ip, fmt::format("{}", port_number));
        asio::connect(host_socket_, udp_endpoints);
    }

    void Control::set_host_port_number(int port_number)
    {
        host_socket_.close();
        host_socket_.open(udp::v4());
        host_socket_.bind(udp::endpoint(udp::v4(), port_number));
    }

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

    void Control::switch_on()
    {
        auto strand = asio::make_strand(io_context_);

        const auto send_data = CommandAcqOn{};

        auto send_action = [this, &send_data]() -> asio::awaitable<void>
        {
            fmt::print("Sending data\n");
            uint32_t counter = 0x80000000;
            auto data_size =
                co_await host_socket_.async_send(serialize(output_buffer_, counter, send_data), asio::use_awaitable);
            fmt::print("waiting....\n");
            fmt::print("vector: {:0x}", fmt::join(output_buffer_, ", "));
            fmt::print("Sending data size: {}\n", data_size);

            auto receive_data_size =
                co_await host_socket_.async_receive(asio::buffer(read_message_buffer_), asio::use_awaitable);
            fmt::print("Received data size: {} from {:?}",
                       receive_data_size,
                       std::string_view{ read_message_buffer_.data(), receive_data_size });
        };

        co_spawn(io_context_, send_action(), asio::detached);
        fmt::print("Comming here\n");

        io_context_.run();
    }

} // namespace srs
