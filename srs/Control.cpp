#include "Control.hpp"
#include "utils/Serializer.hpp"
#include <string_view>
#include <thread>

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
        uint32_t counter = 0x80000000;
        uint16_t zero_0 = 0U;
        uint16_t address = 0x000f;
        uint8_t cmd = 0xaa;
        uint8_t type = 0xaa;
        uint16_t cmd_length = 0xffff;
        std::array<uint32_t, 3> data{ 0U, 15U, 1U };
    };

    struct CommandAcqOff
    {
        uint32_t counter = 0x80000000;
        uint16_t zero_0 = 0U;
        uint16_t address = 0x000f;
        uint8_t cmd = 0xaa;
        uint8_t type = 0xaa;
        uint16_t cmd_length = 0xffff;
        std::array<uint32_t, 3> data{ 0U, 15U, 0U };
    };

    struct CommandReset
    {
        uint32_t counter = 0x80000000;
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

        auto send_action = [this, &send_data]()
        {
            fmt::print("Sending data\n");
            host_socket_.async_send(serialize(send_data, output_buffer_),
                                    [this](std::error_code err, std::size_t data_size)
                                    {
                                        if (data_size > 0)
                                        {
                                            fmt::print("waiting....\n");
                                            std::this_thread::sleep_for(std::chrono::seconds(2));
                                            fmt::print("Sending data size: {}\n", data_size);
                                            // host_socket_.async_receive(
                                            //     asio::buffer(read_message_buffer_),
                                            //     [this](std::error_code err, std::size_t data_size)
                                            //     {
                                            //         fmt::print(
                                            //             "Read data size: {}, with message: {}",
                                            //             data_size,
                                            //             std::string_view(read_message_buffer_.data(), data_size));
                                            //     });
                                        }
                                    });
        };
        // asio::post(asio::bind_executor(strand, send_action));
        // send_action();
        // send_action();
        asio::post(strand, send_action);
        asio::post(strand, send_action);
        // asio::post(send_action);

        io_context_.run();
    }

} // namespace srs
