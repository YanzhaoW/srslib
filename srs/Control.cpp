#include "Control.hpp"
#include <Connection.hpp>
#include <fmt/ranges.h>
#include <string_view>

namespace srs
{
    Control::Control() { start_work(); }

    Control::~Control() { monitoring_thread_.join(); }

    void Control::start_work()
    {
        auto monitoring_action = [this]()
        {
            auto work = asio::make_work_guard(io_context_);
            asio::signal_set signals(io_context_, SIGINT, SIGTERM);
            signals.async_wait(
                [&work, this](auto, auto)
                {
                    fmt::print("Switching it off\n");
                    switch_off();
                    work.reset();
                });
            io_context_.run();
        };
        monitoring_thread_ = std::jthread{ monitoring_action };
    }

    void Control::set_remote_endpoint(std::string_view remote_ip, int port_number)
    {
        auto resolver = udp::resolver{ io_context_ };
        fmt::print("Connecting to socket with ip: {} and port: {}\n", remote_ip, port_number);
        auto udp_endpoints = resolver.resolve(udp::v4(), remote_ip, fmt::format("{}", port_number));
        remote_endpoint_ = *udp_endpoints.begin();
    }

    void Control::switch_on()
    {
        auto connection_info = ConnectionInfo{ this };
        connection_info.local_port_number = default_port1_number_;
        connection_info.endpoint = &remote_endpoint_;
        auto connection = std::make_shared<Starter>(connection_info);
        connection->acq_on();
    }

    void Control::switch_off()
    {
        auto connection_info = ConnectionInfo{ this };
        connection_info.local_port_number = default_port1_number_;
        connection_info.endpoint = &remote_endpoint_;
        auto connection = std::make_shared<Stopper>(connection_info);
        connection->acq_off();
    }
} // namespace srs
