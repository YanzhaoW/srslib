#pragma once

#include "gsl/gsl-lite.hpp"
#include "utils/Serializer.hpp"
#include <asio.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <srs/CommonDefitions.hpp>
#include <srs/Control.hpp>

namespace srs
{
    using udp = asio::ip::udp;

    template <typename T>
    struct is_shared_ptr : std::false_type
    {
    };
    template <typename T>
    struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
    {
    };

    template <typename T>
    concept SharedPtr = is_shared_ptr<T>::value;

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
    template <int buffer_size = SMALL_READ_MSG_BUFFER_SIZE>
    class ConnectionBase : public std::enable_shared_from_this<ConnectionBase<buffer_size>>
    {
      public:
        explicit ConnectionBase(ConnectionInfo info, std::string name)
            : local_port_number_{ info.local_port_number }
            , name_{ std::move(name) }
            , control_{ info.control }
            , endpoint_{ info.endpoint }
        {
            spdlog::debug("Creating connection {} with buffer size: {}", name_, buffer_size);
        }

        void listen(this auto&& self, bool is_continuous = false)
        {
            using asio::experimental::awaitable_operators::operator||;
            spdlog::debug(
                "Connection {}: creating socket with local port number: {}", self.name_, self.local_port_number_);
            self.socket_ = self.new_shared_socket(self.local_port_number_);
            co_spawn(
                self.control_->get_io_context(),
                listen_message(std::static_pointer_cast<std::remove_cvref_t<decltype(self)>>(self.shared_from_this()),
                               is_continuous) ||
                    signal_handling(&self),
                asio::detached);
            spdlog::debug("Connection {}: spawned listen coroutine", self.name_);
        }

        void communicate(this auto&& self, const std::vector<EntryType>& data, uint16_t address)
        {
            self.listen();
            if (self.endpoint_ != nullptr)
            {
                self.encode_write_msg(data, address);
                co_spawn(self.control_->get_io_context(), send_message(self.shared_from_this()), asio::detached);
                spdlog::debug("Connection {}: spawned write coroutine", self.name_);
            }
        }

        void close_socket()
        {
            spdlog::trace("Connection {}: Closing the socket ...", name_);
            socket_->close();
            spdlog::trace("Connection {}: Socket is closed.", name_);
        }

        void end_of_read() { spdlog::trace("calling base class end_of_read from {:?}!", name_); };
        void read_data_handle(std::span<char> read_data) {}

        [[nodiscard]] auto get_read_msg_buffer() const -> const auto& { return read_msg_buffer_; }
        [[nodiscard]] auto get_name() const -> const auto& { return name_; }
        [[nodiscard]] auto get_control() -> auto& { return *control_; }

      private:
        int local_port_number_ = 0;
        uint32_t counter_ = INIT_COUNT_VALUE;
        std::string name_ = "ConnectionBase";
        gsl::not_null<Control*> control_;
        std::unique_ptr<udp::socket> socket_;
        udp::endpoint* endpoint_;
        WriteBufferType write_msg_buffer_;
        ReadBufferType<buffer_size> read_msg_buffer_{};

        static auto send_message(std::shared_ptr<ConnectionBase> connection) -> asio::awaitable<void>;

        void encode_write_msg(const std::vector<EntryType>& data, uint16_t address)
        {
            serialize(write_msg_buffer_,
                      counter_,
                      ZERO_UINT16_PADDING,
                      address,
                      WRITE_COMMAND_BITS,
                      DEFAULT_TYPE_BITS,
                      COMMAND_LENGTH_BITS);
            for (const auto entry : data)
            {
                serialize(write_msg_buffer_, entry);
            }
        }

        auto new_shared_socket(int port_number) -> std::unique_ptr<udp::socket>
        {
            return std::make_unique<udp::socket>(
                control_->get_io_context(), udp::endpoint{ udp::v4(), static_cast<asio::ip::port_type>(port_number) });
        }

        inline void reset_read_msg_buffer() { std::fill(read_msg_buffer_.begin(), read_msg_buffer_.end(), 0); }

        static auto signal_handling(auto* connection) -> asio::awaitable<void>
        {
            auto interrupt_signal = asio::signal_set(co_await asio::this_coro::executor, SIGINT);
            spdlog::trace("Connection {}: waiting for signals", connection->get_name());
            auto [error, sig_num] = co_await interrupt_signal.async_wait(as_tuple(asio::use_awaitable));
            if (error)
            {
                spdlog::trace("Connection {}: Signal ended with {}", connection->get_name(), error.message());
            }
            else
            {
                spdlog::trace("Connection {}: Signal with num {} is called!", sig_num, connection->get_name());
                connection->end_of_read();
            }
        }

        static auto listen_message(SharedPtr auto connection, bool is_continuous = false) -> asio::awaitable<void>;
    };

    template <int size>
    auto ConnectionBase<size>::send_message(std::shared_ptr<ConnectionBase<size>> connection) -> asio::awaitable<void>
    {
        spdlog::debug("Connection {}: Sending data ...", connection->get_name());
        auto data_size = co_await connection->socket_->async_send_to(
            asio::buffer(connection->write_msg_buffer_), *(connection->endpoint_), asio::use_awaitable);
        spdlog::debug("Connection {}: {} bytes data sent with {:02x}",
                      connection->get_name(),
                      data_size,
                      fmt::join(connection->write_msg_buffer_, " "));
    }

    template <int size>
    auto ConnectionBase<size>::listen_message(SharedPtr auto connection, bool is_continuous) -> asio::awaitable<void>
    {
        spdlog::debug("Connection {}: starting to listen ...", connection->get_name());
        while (true)
        {
            auto receive_data_size = co_await connection->socket_->async_receive(
                asio::buffer(connection->read_msg_buffer_), asio::use_awaitable);
            connection->reset_read_msg_buffer();
            if (not is_continuous or connection->get_control().get_status().is_acq_off.load())
            {
                break;
            }
        }
        connection->end_of_read();
    }
} // namespace srs
