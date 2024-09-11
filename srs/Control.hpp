#pragma once

#include <asio/ip/udp.hpp>
#include <asio/signal_set.hpp>
#include <asio/thread_pool.hpp>
#include <atomic>
#include <spdlog/spdlog.h>
#include <srs/CommonDefitions.hpp>
#include <srs/devices/Fec.hpp>
#include <thread>

namespace srs
{
    class DataProcessor;
    struct Status
    {
        std::atomic<bool> is_configured = false;
        std::atomic<bool> is_acq_on = false;
        std::atomic<bool> is_reading = false;
        std::atomic<bool> is_acq_off = false;
        std::condition_variable status_change;

        void wait_for_status(auto&& condition, std::chrono::seconds time_duration = DEFAULT_STATUS_WAITING_TIME_SECONDS)
        {
            auto mutex = std::mutex{};
            while (not condition(*this))
            {
                auto lock = std::unique_lock<std::mutex>{ mutex };
                auto res = status_change.wait_for(lock, time_duration);
                if (res == std::cv_status::timeout)
                {
                    throw std::runtime_error("TIMEOUT");
                }
            }
        }
    };

    class Control
    {
      public:
        Control();

        Control(const Control&) = delete;
        Control(Control&&) = delete;
        Control& operator=(const Control&) = delete;
        Control& operator=(Control&&) = delete;
        ~Control();

        void configure_fec() {}
        void switch_on();
        void switch_off();
        void notify_status_change() { status_.status_change.notify_all(); }
        void run();
        void stop();
        void read_data();
        void abort() {}
        void wait_for_status(auto&& condition, std::chrono::seconds time_duration = DEFAULT_STATUS_WAITING_TIME_SECONDS)
        {
            status_.wait_for_status(std::forward<decltype(condition)>(condition), time_duration);
        }

        // setters:
        void set_remote_endpoint(std::string_view remote_ip, int port_number);
        void set_status_acq_on(bool val = true) { status_.is_acq_on.store(val); }
        void set_status_is_reading(bool val = true) { status_.is_reading.store(val); }

        // getters:
        [[nodiscard]] auto get_channel_address() const -> uint16_t { return channel_address_; }
        [[nodiscard]] auto get_fec_config() const -> const auto& { return fec_config_; }
        [[nodiscard]] auto get_status() const -> const auto& { return status_; }
        [[nodiscard]] auto get_io_context() -> auto& { return io_context_; }

      private:
        using udp = asio::ip::udp;
        static constexpr int default_port1_number_ = 6007;

        Status status_;
        uint16_t channel_address_ = 0xff;
        fec::Config fec_config_;
        std::unique_ptr<DataProcessor> data_processor_;
        io_context_type io_context_{ 4 };
        asio::executor_work_guard<io_context_type::executor_type> io_work_guard_;
        asio::signal_set signal_set_{ io_context_, SIGINT, SIGTERM };
        std::jthread working_thread_;
        udp::endpoint remote_endpoint_;

        void start_work();
    };
} // namespace srs
