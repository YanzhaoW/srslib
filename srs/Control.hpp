#pragma once

// #include <Fec.hpp>
#include "CommonDefitions.hpp"
#include <asio.hpp>

namespace srs
{
    class Control
    {
      public:
        Control() = default;
        void configure_fec() {}
        void switch_on();
        void switch_off() {}
        void set_remote_endpoint(std::string_view remote_ip, int port_number);
        void set_host_port_number(int port_number);

      private:
        using udp = asio::ip::udp;
        static constexpr int default_port_number_ = 6006;

        // fec::Device fec_device_;
        asio::io_context io_context_;
        BufferType output_buffer_;
        udp::socket host_socket_{ io_context_, udp::endpoint{ udp::v4(), default_port_number_ } };
        // udp::socket host_socket_{ io_context_ };
        std::array<char, 1000> read_message_buffer_{};
    };
} // namespace srs
