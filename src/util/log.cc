#include "log.hh"

logging::Logger glogger = logging::Logger();

void
matar::set_log_level(LogLevel level) {
    glogger.set_level(level);
}
