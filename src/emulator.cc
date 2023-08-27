#include "emulator.hh"
#include "bus.hh"
#include "cpu/cpu.hh"
#include <fstream>

namespace emulator {
void
run(std::ifstream& ifile) {
    Bus bus(ifile);
    Cpu cpu(bus);
}

void
run(std::string filepath) {
    std::ifstream ifile(filepath, std::ios::in | std::ios::binary);

    if (!ifile.is_open()) {
        throw std::ios::failure("No such file exists", std::error_code());
    }

    run(ifile);
    ifile.close();
}
}
