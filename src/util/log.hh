#pragma once

#include <iomanip>
#include <iostream>
#include <source_location>
#include <streambuf>

namespace logging {
enum class Level {
    Debug,
    Info,
    Warn,
    Error,
};

class Logger {
  public:
    Logger(std::ostream& os = std::clog);

    template<typename... Args>
    void print(Level level, Args&&... args) {
        set_level(level);
        (os << ... << args);
        reset_level();
        os << std::endl;
    }

  private:
    std::ostream& os;
    void set_level(Level level);
    void reset_level();
};
}

extern logging::Logger logger;

#define log_debug(...) logger.print(logging::Level::Debug, __VA_ARGS__)
#define log_info(...) logger.print(logging::Level::Info, __VA_ARGS__)
#define log_warn(...) logger.print(logging::Level::Warn, __VA_ARGS__)
#define log_error(...) logger.print(logging::Level::Error, __VA_ARGS__)
#define log(...) log_info(__VA_ARGS__)
#define debug(value) log_debug(#value, " = ", value)
