#pragma once

#include <map>
#include <spdlog/common.h>
#include <string>
#include <srs/Application.hpp>

using enum srs::DataPrintMode;
const auto spd_log_map = std::map<std::string, spdlog::level::level_enum>{
    { "trace", spdlog::level::trace }, { "debug", spdlog::level::debug }, { "info", spdlog::level::info },
    { "warn", spdlog::level::warn },   { "error", spdlog::level::err },   { "critical", spdlog::level::critical },
    { "off", spdlog::level::off }
};

const auto print_mode_map = std::map<std::string, srs::DataPrintMode>{ { "speed", print_speed },
                                                                       { "header", print_header },
                                                                       { "raw", print_raw },
                                                                       { "all", print_all } };
