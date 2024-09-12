#include "CLI/CLI.hpp"
#include "spdlog/spdlog.h"
#include <srs/Control.hpp>

auto main(int argc, char** argv) -> int
{
    auto cli_args = CLI::App{ "SRS system command line interface" };
    argv = cli_args.ensure_utf8(argv);

    auto spd_log_map = std::map<std::string, spdlog::level::level_enum>{
        { "trace", spdlog::level::trace }, { "debug", spdlog::level::debug }, { "info", spdlog::level::info },
        { "warn", spdlog::level::warn },   { "error", spdlog::level::err },   { "critical", spdlog::level::critical },
        { "off", spdlog::level::off }
    };

    using enum srs::DataPrintMode;
    auto print_mode_map = std::map<std::string, srs::DataPrintMode>{
        { "speed", print_speed }, { "header", print_header }, { "raw", print_raw }, { "all", print_all }
    };

    auto spdlog_level = spdlog::level::info;
    auto print_mode = print_speed;

    cli_args.add_option("-v, --verbose-level", spdlog_level, "set log level")
        ->transform(CLI::CheckedTransformer(spd_log_map, CLI::ignore_case))
        ->capture_default_str();
    cli_args.add_option("-p, --print-mode", print_mode, "set print mode")
        ->transform(CLI::CheckedTransformer(print_mode_map, CLI::ignore_case))
        ->capture_default_str();

    CLI11_PARSE(cli_args, argc, argv);

    spdlog::set_level(spdlog_level);
    spdlog::set_pattern("[%H:%M:%S] [%^%=7l%$] [thread %t] %v");

    try
    {
        spdlog::info("Welcome to SRS program");
        auto control = srs::Control{};

        control.set_remote_endpoint("10.0.0.2", 6600);
        control.set_print_mode(print_mode);

        control.read_data();
        control.switch_on();

        control.run();
    }
    catch (std::exception& ex)
    {
        spdlog::critical("Exception occured: {}", ex.what());
    }

    return 0;
}
