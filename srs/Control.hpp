#pragma once

#include <asio.hpp>
#include <srs/CommandHandlers.hpp>
#include <srs/CommonDefitions.hpp>
#include <srs/devices/Fec.hpp>

namespace srs
{
    class Control
    {
      public:
        Control() = default;
        void configure_fec();
        void switch_on();
        void switch_off() {}
        void set_remote_endpoint(std::string_view remote_ip, int port_number);
        void set_host_port_number(int port_number);

        [[nodiscard]] auto get_channel_address() const -> uint16_t { return channel_address_; }
        [[nodiscard]] auto get_fec_config() const -> const auto& { return fec_config_; }

        template <MessageMode mode, typename DataType>
        void register_command(const DataType& data, uint16_t address)
        {
        }

        template <MessageMode mode, typename DataType>
        void register_command(const DataType& data)
        {
            register_command(data, get_channel_address());
        }

      private:
        using udp = asio::ip::udp;
        static constexpr int default_port_number_ = 6006;

        fec::Config fec_config_;
        asio::io_context io_context_;
        BufferType output_buffer_;
        udp::socket host_socket_{ io_context_, udp::endpoint{ udp::v4(), default_port_number_ } };
        // udp::socket host_socket_{ io_context_ };
        std::array<char, 1000> read_message_buffer_{};
        uint16_t channel_address_ = 0xff;
    };
} // namespace srs
