#include "DataProcessor.hpp"
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <srs/Control.hpp>

namespace srs
{
    DataMonitor::DataMonitor(DataProcessor* processor, io_context_type* io_context)
        : processor_{ processor }
        , io_context_{ io_context }
        , clock_{ *io_context_ }
    {
    }

    auto DataMonitor::print_cycle() -> asio::awaitable<void>
    {
        clock_.expires_after(period_);
        console_ = spdlog::stdout_color_st("Data Monitor");
        auto console_format = std::make_unique<spdlog::pattern_formatter>(
            "[%H:%M:%S] <%n> %v", spdlog::pattern_time_type::local, std::string(""));
        console_->set_formatter(std::move(console_format));
        console_->flush_on(spdlog::level::info);
        console_->set_level(spdlog::level::info);
        while (true)
        {
            co_await clock_.async_wait(asio::use_awaitable);
            clock_.expires_after(period_);

            auto total_bytes_count = processor_->get_read_data_bytes();
            auto time_now = std::chrono::steady_clock::now();

            auto time_duration = std::chrono::duration_cast<std::chrono::microseconds>(time_now - last_print_time_);
            auto bytes_read = static_cast<double>(total_bytes_count - last_read_data_bytes_);

            last_read_data_bytes_ = total_bytes_count;
            last_print_time_ = time_now;

            set_speed_string(bytes_read / static_cast<double>(time_duration.count()));
            console_->info("Data reading rate: {}. Press \"Ctrl-C\" to stop.\r", speed_string_);
        }
    }

    void DataMonitor::set_speed_string(double speed_MBps)
    {
        if (speed_MBps < 1.)
        {
            speed_string_ =
                fmt::format(fg(fmt::color::yellow) | fmt::emphasis::bold, "{:>7.5} KB/s", 1000. * speed_MBps);
        }
        else
        {
            speed_string_ = fmt::format(fg(fmt::color::yellow) | fmt::emphasis::bold, "{:>7.5} MB/s", speed_MBps);
        }
    }

    void DataMonitor::start() { asio::co_spawn(*io_context_, print_cycle(), asio::detached); }
    void DataMonitor::stop() { clock_.cancel(); }

    DataProcessor::DataProcessor(Control* control)
        : control_{ control }
        , monitor_{ this, &control_->get_io_context() }
    {
    }

    void DataProcessor::start()
    {
        is_stopped.store(false);
        monitor_.start();
        asio::post(control_->get_io_context(), [this]() { analysis_loop(); });
    }

    void DataProcessor::stop()
    {
        // CAS operation to guarantee the thread safty
        auto expected = false;
        if (is_stopped.compare_exchange_strong(expected, true))
        {
            monitor_.stop();
            data_queue_.abort();
        }
    }

    void DataProcessor::read_data_once(std::span<BufferElementType> read_data)
    {
        total_read_data_bytes_ += read_data.size();
        auto is_success = data_queue_.try_emplace(read_data);
        if (not is_success)
        {
            spdlog::critical("Data queue is full and message is lost. Try to increase its capacity!");
        }
    }

    void DataProcessor::analysis_loop()
    {
        try
        {
            spdlog::trace("entering analysis loop");
            auto input_data_buffer = SerializableMsgBuffer{};

            while (not is_stopped)
            {
                data_queue_.pop(input_data_buffer);
            }
        }
        catch (oneapi::tbb::user_abort& ex)
        {
            spdlog::info("Data processing: {}", ex.what());
        }
        catch (std::exception& ex)
        {
            // TODO: call stop of the control
            spdlog::critical(ex.what());
            control_->exit();
        }
    }
} // namespace srs
