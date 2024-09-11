#include "Control.hpp"
#include "DataProcessor.hpp"
#include "spdlog/spdlog.h"
#include <Connections.hpp>
#include <fmt/ranges.h>
#include <string_view>

namespace srs
{
    Control::Control()
        : io_work_guard_{ asio::make_work_guard(io_context_) }
    {
        data_processor_ = std::make_unique<DataProcessor>(this);
        start_work();
    }

    Control::~Control() = default;

    void Control::start_work()
    {
        auto monitoring_action = [this]()
        {
            // auto work = asio::make_work_guard(io_context_);
            asio::signal_set signals(io_context_, SIGINT, SIGTERM);
            signals.async_wait(
                [this](auto, auto)
                {
                    spdlog::trace("calling SIGINT from monitoring thread");
                    stop();
                });
            io_context_.join();
        };
        working_thread_ = std::jthread{ monitoring_action };
    }

    void Control::stop()
    {
        data_processor_->stop();
        spdlog::debug("Turning srs system off ...");
        status_.is_acq_off.store(true);
        status_.wait_for_status([](const auto& status) { return not status.is_reading.load(); });

        switch_off();
        set_status_acq_on(false);
        spdlog::info("SRS system is turned off");
        io_work_guard_.reset();
    }

    void Control::set_remote_endpoint(std::string_view remote_ip, int port_number)
    {
        auto resolver = udp::resolver{ io_context_ };
        spdlog::debug("Set the remote socket with ip: {} and port: {}", remote_ip, port_number);
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

    void Control::read_data()
    {
        auto connection_info = ConnectionInfo{ this };
        connection_info.local_port_number = FEC_DAQ_RECEIVE_PORT;
        auto data_reader = std::make_shared<DataReader>(connection_info, data_processor_.get());
        data_reader->start();
    }

    void Control::run()
    {
        data_processor_->start();
        working_thread_.join();
    }
} // namespace srs
