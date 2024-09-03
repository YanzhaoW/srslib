#include "Connection.hpp"
#include "utils/Serializer.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <ranges>

namespace srs
{
    void ConnectionBase::encode_write_msg(const std::vector<EntryType>& data, uint16_t address)
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

    auto ConnectionBase::listen_message(std::shared_ptr<ConnectionBase> connection) -> asio::awaitable<void>
    {
        fmt::print("starting to listen.....\n");
        auto receive_data_size = co_await connection->socket_->async_receive(asio::buffer(connection->read_msg_buffer_),
                                                                             asio::use_awaitable);
        fmt::print(
            "Received data size: {} from {:#04x}\n",
            receive_data_size,
            fmt::join(std::ranges::views::take(connection->read_msg_buffer_, static_cast<int>(receive_data_size)),
                      ", "));
    }

    auto ConnectionBase::send_message(std::shared_ptr<ConnectionBase> connection) -> asio::awaitable<void>
    {
        fmt::print("Sending data ...\n");
        auto data_size = co_await connection->socket_->async_send_to(
            asio::buffer(connection->write_msg_buffer_), *(connection->endpoint_), asio::use_awaitable);
        fmt::print("vector: {:0x}\n", fmt::join(connection->write_msg_buffer_, ", "));
        fmt::print("Sending data size: {}\n", data_size);
    }

    void ConnectionBase::communicate(const std::vector<EntryType>& data, uint16_t address)
    {
        encode_write_msg(data, address);
        co_spawn(*io_context_, listen_message(shared_from_this()), asio::detached);
        co_spawn(*io_context_, send_message(shared_from_this()), asio::detached);
    }
} // namespace srs
