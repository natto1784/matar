#include "../../src/gdb_rsp.hh"
#include "bus.hh"
#include "cpu/cpu.hh"
#include "util/loglevel.hh"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// NOLINTBEGIN

int
main(int argc, const char* argv[]) {
    std::vector<uint8_t> rom;
    std::array<uint8_t, matar::Bus::BIOS_SIZE> bios = { 0 };
    uint64_t cycles;

    auto usage = [argv]() {
        std::cerr << "Usage: " << argv[0] << " <file> [-b <bios>]" << std::endl;
        std::exit(EXIT_FAILURE);
    };

    if (argc < 2)
        usage();

    std::string rom_file, bios_file = "gba_bios.bin";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-b") {
            if (++i < argc)
                bios_file = argv[i];
            else
                usage();
        } else if (arg == "-c") {
            if (++i < argc)
                cycles = std::stoull(argv[i]);
            else
                usage();
        } else {
            rom_file = arg;
        }
    }
    std::cout << cycles << std::endl;

    if (rom_file.empty())
        usage();

    try {
        std::ifstream ifile(rom_file, std::ios::in | std::ios::binary);
        std::streampos bios_size;

        if (!ifile.is_open()) {
            throw std::ios::failure("File not found", std::error_code());
        }

        rom.assign(std::istreambuf_iterator<char>(ifile),
                   std::istreambuf_iterator<char>());

        ifile.close();

        ifile.open(bios_file, std::ios::in | std::ios::binary);

        if (!ifile.is_open()) {
            throw std::ios::failure("BIOS file not found", std::error_code());
        }

        ifile.seekg(0, std::ios::end);
        bios_size = ifile.tellg();

        if (bios_size != matar::Bus::BIOS_SIZE) {
            throw std::ios::failure("BIOS file has invalid size",
                                    std::error_code());
        }

        ifile.seekg(0, std::ios::beg);

        ifile.read(reinterpret_cast<char*>(bios.data()), bios.size());

        ifile.close();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    std::flush(std::cout);
    std::flush(std::cout);


    try {
        matar::Bus bus = matar::Bus(std::move(bios), std::move(rom));
        matar::Cpu cpu(bus);

        bus.run(cycles);

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// NOLINTEND
