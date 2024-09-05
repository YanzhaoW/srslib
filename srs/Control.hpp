#pragma once

#include <asio.hpp>
#include <atomic>
#include <srs/CommandHandlers.hpp>
#include <srs/CommonDefitions.hpp>
#include <srs/devices/Fec.hpp>
#include <thread>

namespace srs
{
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

        void configure_fec() {}
        void switch_on();
        void switch_off();
        void notify_status_change() { status_.status_change.notify_all(); }
        void run() { monitoring_thread_.join(); }
        void abort() {}

        // setters:
        void set_remote_endpoint(std::string_view remote_ip, int port_number);
        void set_status_acq_on(bool val = true) { status_.is_acq_on.store(val); }

        // getters:
        [[nodiscard]] auto get_channel_address() const -> uint16_t { return channel_address_; }
        [[nodiscard]] auto get_fec_config() const -> const auto& { return fec_config_; }
        [[nodiscard]] auto get_status() -> auto& { return status_; }
        [[nodiscard]] auto get_io_context() -> auto& { return io_context_; }

      private:
        using udp = asio::ip::udp;
        static constexpr int default_port1_number_ = 6007;

        Status status_;
        fec::Config fec_config_;
        asio::io_context io_context_;
        BufferType output_buffer_;
        std::jthread monitoring_thread_;
        // udp::socket listen_socket_;
        udp::endpoint remote_endpoint_;
        // std::array<char, 1000> read_message_buffer_{};
        uint16_t channel_address_ = 0xff;

        void start_work();
    };
} // namespace srs
