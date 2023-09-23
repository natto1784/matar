#pragma once

#include "util/loglevel.hh"
#include <fmt/ostream.h>
#include <iostream>

namespace logging {
namespace ansi {
static constexpr std::string_view RED     = "\033[31m";
static constexpr std::string_view YELLOW  = "\033[33m";
static constexpr std::string_view MAGENTA = "\033[35m";
static constexpr std::string_view WHITE   = "\033[37m";
static constexpr std::string_view BOLD    = "\033[1m";
static constexpr std::string_view RESET   = "\033[0m";
}

using fmt::print;

class Logger {
    using LogLevel = matar::LogLevel;

  public:
    Logger(LogLevel level = LogLevel::Debug)
      : level(0) {
        set_level(level);
    }

    template<typename... Args>
    void log(const fmt::format_string<Args...>& fmt, Args&&... args) {
        fmt::println(stream, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const fmt::format_string<Args...>& fmt, Args&&... args) {
        if (level & static_cast<uint8_t>(LogLevel::Debug)) {
            print(stream, "{}{}[DEBUG] ", ansi::MAGENTA, ansi::BOLD);
            log(fmt, std::forward<Args>(args)...);
            print(stream, ansi::RESET);
        }
    }

    template<typename... Args>
    void info(const fmt::format_string<Args...>& fmt, Args&&... args) {
        if (level & static_cast<uint8_t>(LogLevel::Info)) {
            print(stream, "{}[INFO] ", ansi::WHITE);
            log(fmt, std::forward<Args>(args)...);
            print(stream, ansi::RESET);
        }
    }

    template<typename... Args>
    void warn(const fmt::format_string<Args...>& fmt, Args&&... args) {
        if (level & static_cast<uint8_t>(LogLevel::Warn)) {
            print(stream, "{}[WARN] ", ansi::YELLOW);
            log(fmt, std::forward<Args>(args)...);
            print(stream, ansi::RESET);
        }
    }

    template<typename... Args>
    void error(const fmt::format_string<Args...>& fmt, Args&&... args) {
        if (level & static_cast<uint8_t>(LogLevel::Error)) {
            print(stream, "{}{}[ERROR] ", ansi::RED, ansi::BOLD);
            log(fmt, std::forward<Args>(args)...);
            print(stream, ansi::RESET);
        }
    }

    void set_level(LogLevel level) {
        this->level = (static_cast<uint8_t>(level) << 1) - 1;
    }
    void set_stream(std::ostream& stream) { this->stream = stream; }

  private:
    uint8_t level;
    std::reference_wrapper<std::ostream> stream = std::clog;
};
}

extern logging::Logger glogger;

#define debug(x) logger.debug("{} = {}", #x, x);
