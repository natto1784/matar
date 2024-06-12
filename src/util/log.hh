#pragma once

#include "util/loglevel.hh"
#include <print>

namespace logging {
namespace ansi {
static constexpr auto RED     = "\033[31m";
static constexpr auto YELLOW  = "\033[33m";
static constexpr auto MAGENTA = "\033[35m";
static constexpr auto WHITE   = "\033[37m";
static constexpr auto BOLD    = "\033[1m";
static constexpr auto RESET   = "\033[0m";
}

using std::print;

class Logger {
    using LogLevel = matar::LogLevel;

  public:
    Logger(LogLevel level = LogLevel::Debug, FILE* stream = stdout)
      : level(0)
      , stream(stream) {
        set_level(level);
    }

    template<typename... Args>
    void log(const std::format_string<Args...>& fmt, Args&&... args) {
        std::println(stream, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const std::format_string<Args...>& fmt, Args&&... args) {
        if (level & static_cast<uint8_t>(LogLevel::Debug)) {
            print(stream, "{}{}[DEBUG] ", ansi::MAGENTA, ansi::BOLD);
            log(fmt, std::forward<Args>(args)...);
            print(stream, ansi::RESET);
        }
    }

    template<typename... Args>
    void info(const std::format_string<Args...>& fmt, Args&&... args) {
        if (level & static_cast<uint8_t>(LogLevel::Info)) {
            print(stream, "{}[INFO] ", ansi::WHITE);
            log(fmt, std::forward<Args>(args)...);
            print(stream, ansi::RESET);
        }
    }

    template<typename... Args>
    void info_bold(const std::format_string<Args...>& fmt, Args&&... args) {
        if (level & static_cast<uint8_t>(LogLevel::Info)) {
            print(stream, "{}{}[INFO] ", ansi::WHITE, ansi::BOLD);
            log(fmt, std::forward<Args>(args)...);
            print(stream, ansi::RESET);
        }
    }

    template<typename... Args>
    void warn(const std::format_string<Args...>& fmt, Args&&... args) {
        if (level & static_cast<uint8_t>(LogLevel::Warn)) {
            print(stream, "{}[WARN] ", ansi::YELLOW);
            log(fmt, std::forward<Args>(args)...);
            print(stream, ansi::RESET);
        }
    }

    template<typename... Args>
    void error(const std::format_string<Args...>& fmt, Args&&... args) {
        if (level & static_cast<uint8_t>(LogLevel::Error)) {
            print(stream, "{}{}[ERROR] ", ansi::RED, ansi::BOLD);
            log(fmt, std::forward<Args>(args)...);
            print(stream, ansi::RESET);
        }
    }

    void set_level(LogLevel level) {
        this->level = (static_cast<uint8_t>(level) << 1) - 1;
    }
    void set_stream(FILE* stream) { this->stream = stream; }

  private:
    uint8_t level;
    FILE* stream;
};
}

extern logging::Logger glogger;

#define dbg(x) glogger.debug("{} = {}", #x, x);
