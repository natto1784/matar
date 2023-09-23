#pragma once

namespace matar {
enum class LogLevel {
    Off   = 1 << 0,
    Error = 1 << 1,
    Warn  = 1 << 2,
    Info  = 1 << 3,
    Debug = 1 << 4
};

void
set_log_level(LogLevel level);
}
