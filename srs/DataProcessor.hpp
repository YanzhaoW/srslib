#pragma once

#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <deque>
#include <gsl/gsl-lite.hpp>
#include <span>
#include <spdlog/logger.h>
#include <srs/CommonDefitions.hpp>

namespace srs
{
    class DataProcessor;
    class Control;

    class DataMonitor
    {
      public:
        explicit DataMonitor(DataProcessor* processor, asio::io_context* io_context);
        void show_data_speed(bool val = true) { is_shown_ = val; }
        void set_display_period(std::chrono::milliseconds duration) { period_ = duration; }
        auto print_cycle() -> asio::awaitable<void>;
        void start();
        void stop();

      private:
        bool is_shown_ = true;
        gsl::not_null<DataProcessor*> processor_;
        gsl::not_null<asio::io_context*> io_context_;
        std::shared_ptr<spdlog::logger> console_;
        asio::steady_timer clock_;
        std::atomic<uint64_t> last_read_data_bytes_ = 0;
        std::chrono::time_point<std::chrono::steady_clock> last_print_time_ = std::chrono::steady_clock::now();
        std::chrono::milliseconds period_ = DEFAULT_DISPLAY_PERIOD;
    };

    class DataProcessor
    {
      public:
        explicit DataProcessor(Control* control);

        // Need to be fast return
        void read_data_once(std::span<BufferElementType> read_data);

        void start() { monitor_.start(); }
        void stop() { monitor_.stop(); }

        // getters:
        [[nodiscard]] auto get_read_data_bytes() const -> uint64_t { return total_read_data_bytes_.load(); }

        // setters:
        void set_show_data_speed(bool val = true) { monitor_.show_data_speed(val); }
        void set_monitor_display_period(std::chrono::milliseconds duration) { monitor_.set_display_period(duration); }

      private:
        std::deque<char> data_queue_;
        std::atomic<uint64_t> total_read_data_bytes_ = 0;
        gsl::not_null<Control*> control_;
        DataMonitor monitor_;
    };

} // namespace srs
