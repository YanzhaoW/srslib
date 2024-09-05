#pragma once

#include "gsl/gsl-lite.hpp"
#include <asio.hpp>
#include <srs/CommonDefitions.hpp>
#include <srs/Control.hpp>

namespace srs
{
    using udp = asio::ip::udp;

    struct ConnectionInfo
    {
        explicit ConnectionInfo(Control* control_ptr)
            : control{ control_ptr }
        {
        }
        Control* control = nullptr;
        int local_port_number = 0;
        udp::endpoint* endpoint = nullptr;
    };

    // derive from enable_shared_from_this to make sure object still alive in the coroutine
    class ConnectionBase : public std::enable_shared_from_this<ConnectionBase>
    {
      public:
        explicit ConnectionBase(ConnectionInfo info)
            : control_{ info.control }
            , local_port_number_{ info.local_port_number }
            , endpoint_{ info.endpoint }
        {
        }

        void communicate(const std::vector<EntryType>& data, uint16_t address);
        void close_socket() { socket_->close(); }

        [[nodiscard]] auto get_read_msg_buffer() const -> const auto& { return read_msg_buffer_; }
        [[nodiscard]] auto get_control() -> auto& { return *control_; }

      private:
        int local_port_number_ = 0;
        uint32_t counter_ = INIT_COUNT_VALUE;
        gsl::not_null<Control*> control_;
        std::unique_ptr<udp::socket> socket_;
        gsl::not_null<udp::endpoint*> endpoint_;
        BufferType write_msg_buffer_;
        std::array<char, READ_MSG_BUFFER_SIZE> read_msg_buffer_{};

        static auto listen_message(std::shared_ptr<ConnectionBase> connection) -> asio::awaitable<void>;
        static auto send_message(std::shared_ptr<ConnectionBase> connection) -> asio::awaitable<void>;
        void encode_write_msg(const std::vector<EntryType>& data, uint16_t address);
        auto new_shared_socket(int port_number) -> std::unique_ptr<udp::socket>;
    };

    class Starter : public ConnectionBase
    {
      public:
        explicit Starter(ConnectionInfo info)
            : ConnectionBase(info)
        {
        }

        Starter(const Starter&) = delete;
        Starter(Starter&&) = default;
        Starter& operator=(const Starter&) = delete;
        Starter& operator=(Starter&&) = default;
        ~Starter();

        void acq_on()
        {
            const auto data = std::vector<EntryType>{ 0, 15, 1 };
            communicate(data, NULL_ADDRESS);
        }
    };

    class Stopper : public ConnectionBase
    {
      public:
        explicit Stopper(ConnectionInfo info)
            : ConnectionBase(info)
        {
        }

        void acq_off();
    };
} // namespace srs
