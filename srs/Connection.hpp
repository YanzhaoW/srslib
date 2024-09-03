#pragma once

#include "gsl/gsl-lite.hpp"
#include <asio.hpp>
#include <srs/CommonDefitions.hpp>

namespace srs
{
    using udp = asio::ip::udp;

    struct ConnectionInfo
    {
        explicit ConnectionInfo(asio::io_context* io_context_ptr)
            : io_context{ io_context_ptr }
        {
        }
        asio::io_context* io_context = nullptr;
        std::shared_ptr<udp::socket> socket;
        udp::endpoint* endpoint = nullptr;
    };

    // derive from enable_shared_from_this to make sure object still alive in the coroutine
    class ConnectionBase : public std::enable_shared_from_this<ConnectionBase>
    {
      public:
        explicit ConnectionBase(ConnectionInfo info)
            : io_context_{ info.io_context }
            , socket_{ std::move(info.socket) }
            , endpoint_{ info.endpoint }
        {
        }

        void communicate(const std::vector<EntryType>& data, uint16_t address);

        [[nodiscard]] auto get_read_msg_buffer() const -> const auto& { return read_msg_buffer_; }

      private:
        uint32_t counter_ = INIT_COUNT_VALUE;
        gsl::not_null<asio::io_context*> io_context_;
        std::shared_ptr<udp::socket> socket_;
        gsl::not_null<udp::endpoint*> endpoint_;
        BufferType write_msg_buffer_;
        std::array<char, READ_MSG_BUFFER_SIZE> read_msg_buffer_{};

        static auto listen_message(std::shared_ptr<ConnectionBase> connection) -> asio::awaitable<void>;
        static auto send_message(std::shared_ptr<ConnectionBase> connection) -> asio::awaitable<void>;
        void encode_write_msg(const std::vector<EntryType>& data, uint16_t address);
    };

    class Starter : public ConnectionBase
    {
      public:
        explicit Starter(ConnectionInfo info)
            : ConnectionBase(std::move(info))
        {
        }

        void acq_on()
        {
            const auto data = std::vector<EntryType>{ 0, 15, 1 };
            communicate(data, NULL_ADDRESS);
        }
    };

} // namespace srs
