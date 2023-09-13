#pragma once

#include <fmt/ostream.h>
#include <iostream>

using fmt::print;
using std::clog;

namespace logger {
namespace ansi {
static constexpr std::string_view RED     = "\033[31m";
static constexpr std::string_view YELLOW  = "\033[33m";
static constexpr std::string_view MAGENTA = "\033[35m";
static constexpr std::string_view WHITE   = "\033[37m";
static constexpr std::string_view BOLD    = "\033[1m";
static constexpr std::string_view RESET   = "\033[0m";
}

template<typename... Args>
inline void
log_raw(const fmt::format_string<Args...>& fmt, Args&&... args) {
    fmt::println(clog, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void
log_debug(const fmt::format_string<Args...>& fmt, Args&&... args) {
    print(clog, "{}{}[DEBUG] ", ansi::MAGENTA, ansi::BOLD);
    log_raw(fmt, std::forward<Args>(args)...);
    print(clog, ansi::RESET);
}

template<typename... Args>
inline void
log_info(const fmt::format_string<Args...>& fmt, Args&&... args) {
    print(clog, "{}[INFO] ", ansi::WHITE);
    log_raw(fmt, std::forward<Args>(args)...);
    print(clog, ansi::RESET);
}

template<typename... Args>
inline void
log_warn(const fmt::format_string<Args...>& fmt, Args&&... args) {
    print(clog, "{}[WARN] ", ansi::YELLOW);
    log_raw(fmt, std::forward<Args>(args)...);
    print(clog, ansi::RESET);
}

template<typename... Args>
inline void
log_error(const fmt::format_string<Args...>& fmt, Args&&... args) {
    print(clog, "{}{}[ERROR] ", ansi::RED, ansi::BOLD);
    log_raw(fmt, std::forward<Args>(args)...);
    print(clog, ansi::RESET);
}
}

#define debug(value) logger::log_debug("{} = {}", #value, value)
