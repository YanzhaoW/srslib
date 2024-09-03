#include "Connection.hpp"
#include "utils/Serializer.hpp"
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <mutex>
#include <ranges>
#include <srs/Control.hpp>

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
        socket_ = new_shared_socket(local_port_number_);
        encode_write_msg(data, address);
        co_spawn(control_->get_io_context(), listen_message(shared_from_this()), asio::detached);
        co_spawn(control_->get_io_context(), send_message(shared_from_this()), asio::detached);
    }

    void ConnectionBase::wait_for_status(std::function<bool(const Status&)> const& condition,
                                         std::chrono::seconds time_duration)
    {
        auto mutex = std::mutex{};
        auto& status = control_->get_status();
        auto& cond_var = status.status_change;
        while (not condition(status))
        {
            auto lock = std::unique_lock<std::mutex>{ mutex };
            auto res = cond_var.wait_for(lock, time_duration);
            if (res == std::cv_status::timeout)
            {
                throw std::runtime_error("TIMEOUT");
            }
        }
    }

    auto ConnectionBase::new_shared_socket(int port_number) -> std::unique_ptr<udp::socket>
    {
        return std::make_unique<udp::socket>(control_->get_io_context(),
                                             udp::endpoint{ udp::v4(), static_cast<asio::ip::port_type>(port_number) });
    }

    Starter::~Starter()
    {
        close_socket();
        auto& control = get_control();
        control.set_status_acq_on();
        control.notify_status_change();
    }

    void Stopper::acq_off()
    {
        const auto waiting_time = std::chrono::seconds{ 10 };
        wait_for_status([](const Status& status) { return status.is_acq_on.load(); }, waiting_time);
        const auto data = std::vector<EntryType>{ 0, 15, 0 };
        communicate(data, NULL_ADDRESS);
    }
} // namespace srs
