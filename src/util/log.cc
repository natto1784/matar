#include "log.hh"

namespace logging {

namespace ansi {
static const char* RED     = "\033[31m";
static const char* YELLOW  = "\033[33m";
static const char* MAGENTA = "\033[35m";
static const char* WHITE   = "\033[37m";
static const char* BOLD    = "\033[1m";
static const char* RESET   = "\033[0m";
}

Logger::Logger(std::ostream& os)
  : os(os){};

void
Logger::set_level(Level level) {
    switch (level) {
        case Level::Debug:
            os << ansi::MAGENTA << ansi::BOLD << "[DEBUG] ";
            break;

        case Level::Info:
            os << ansi::WHITE << "[INFO] ";
            break;

        case Level::Warn:
            os << ansi::YELLOW << "[WARN] ";
            break;

        case Level::Error:
            os << ansi::RED << ansi::BOLD << "[ERROR] ";
            break;

            // unreachable
        default: {
        }
    }
}

void
Logger::reset_level() {
    os << ansi::RESET;
}
}

// Global logger
logging::Logger logger;
