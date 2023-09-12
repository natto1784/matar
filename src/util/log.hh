#pragma once

#include <fmt/ostream.h>
#include <iostream>

namespace logger {
inline std::ostream& stream = std::clog;

namespace ansi {
static constexpr char RED[]     = "\033[31m";
static constexpr char YELLOW[]  = "\033[33m";
static constexpr char MAGENTA[] = "\033[35m";
static constexpr char WHITE[]   = "\033[37m";
static constexpr char BOLD[]    = "\033[1m";
static constexpr char RESET[]   = "\033[0m";
}

template<typename... Args>
inline void
log_raw(const fmt::format_string<Args...>& fmt, Args&&... args) {
    fmt::println(stream, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void
log_debug(const fmt::format_string<Args...>& fmt, Args&&... args) {
    fmt::print(stream, "{}{}[DEBUG] ", ansi::MAGENTA, ansi::BOLD);
    log_raw(fmt, std::forward<Args>(args)...);
    fmt::print(stream, ansi::RESET);
}

template<typename... Args>
inline void
log_info(const fmt::format_string<Args...>& fmt, Args&&... args) {
    fmt::print(stream, "{}[INFO] ", ansi::WHITE);
    log_raw(fmt, std::forward<Args>(args)...);
    fmt::print(stream, ansi::RESET);
}

template<typename... Args>
inline void
log_warn(const fmt::format_string<Args...>& fmt, Args&&... args) {
    fmt::print(stream, "{}[WARN] ", ansi::YELLOW);
    log_raw(fmt, std::forward<Args>(args)...);
    fmt::print(stream, ansi::RESET);
}

template<typename... Args>
inline void
log_error(const fmt::format_string<Args...>& fmt, Args&&... args) {
    fmt::print(stream, "{}{}[ERROR] ", ansi::RED, ansi::BOLD);
    log_raw(fmt, std::forward<Args>(args)...);
    fmt::print(stream, ansi::RESET);
}
}

#define debug(value) logger::log_debug("{} = {}", #value, value)
