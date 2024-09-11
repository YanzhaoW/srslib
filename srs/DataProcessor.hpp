#pragma once

#include <asio/awaitable.hpp>
#include <asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <gsl/gsl-lite.hpp>
#include <span>
#include <spdlog/logger.h>
#include <srs/CommonDefitions.hpp>
#include <srs/utils/Serializer.hpp>
#include <tbb/concurrent_queue.h>

namespace srs
{
    class DataProcessor;
    class Control;

    class DataMonitor
    {
      public:
        explicit DataMonitor(DataProcessor* processor, io_context_type* io_context);
        void show_data_speed(bool val = true) { is_shown_ = val; }
        void set_display_period(std::chrono::milliseconds duration) { period_ = duration; }
        auto print_cycle() -> asio::awaitable<void>;
        void start();
        void stop();

      private:
        bool is_shown_ = true;
        gsl::not_null<DataProcessor*> processor_;
        gsl::not_null<io_context_type*> io_context_;
        std::shared_ptr<spdlog::logger> console_;
        asio::steady_timer clock_;
        std::atomic<uint64_t> last_read_data_bytes_ = 0;
        std::chrono::time_point<std::chrono::steady_clock> last_print_time_ = std::chrono::steady_clock::now();
        std::chrono::milliseconds period_ = DEFAULT_DISPLAY_PERIOD;
        std::string speed_string_;

        void set_speed_string(double speed_MBps);
    };

    class DataProcessor
    {
      public:
        explicit DataProcessor(Control* control);

        // Need to be fast return
        void read_data_once(std::span<BufferElementType> read_data);


        void start();
        void stop();

        // getters:
        [[nodiscard]] auto get_read_data_bytes() const -> uint64_t { return total_read_data_bytes_.load(); }

        // setters:
        void set_show_data_speed(bool val = true) { monitor_.show_data_speed(val); }
        void set_monitor_display_period(std::chrono::milliseconds duration) { monitor_.set_display_period(duration); }

      private:
        std::atomic<bool> is_stopped{ false };
        tbb::concurrent_bounded_queue<SerializableMsgBuffer> data_queue_;
        std::atomic<uint64_t> total_read_data_bytes_ = 0;
        gsl::not_null<Control*> control_;
        DataMonitor monitor_;

        // should run on a different task
        void analysis_loop();
    };

} // namespace srs
