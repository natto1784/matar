#include "util/loglevel.hh"
#include <catch2/catch_session.hpp>

int
main(int argc, char* argv[]) {
    matar::set_log_level(matar::LogLevel::Off);
    return Catch::Session().run(argc, argv);
}
