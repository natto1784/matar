#pragma once

#include "header.hh"
#include "io/io.hh"
#include "memory.hh"
#include "scheduler.hh"
#include <vector>

namespace matar {
class Cpu;

enum CpuAccess {
    Sequential,
    NonSequential
};

struct CycleCount {
    uint8_t n16; // non sequential 8/16 bit width access
    uint8_t n32; // non sequential 32 bit width access
    uint8_t s16; // seuquential 8/16 bit width access
    uint8_t s32; // sequential 32 bit width access
};

class Bus {
  public:
    static constexpr uint32_t BIOS_SIZE = 1024 * 16;

    Bus(std::array<uint8_t, BIOS_SIZE>&&, std::vector<uint8_t>&&);

    void attach_cpu(Cpu* c) { cpu = c; }

    void update_cycle_map(WaitstateControl waitcnt);

    void step();

    uint8_t read_byte(uint32_t address,
                      CpuAccess access = CpuAccess::Sequential);
    void write_byte(uint32_t address,
                    uint8_t byte,
                    CpuAccess access = CpuAccess::Sequential);

    uint16_t read_halfword(uint32_t address,
                           CpuAccess access = CpuAccess::Sequential);
    void write_halfword(uint32_t address,
                        uint16_t halfword,
                        CpuAccess access = CpuAccess::Sequential);

    uint32_t read_word(uint32_t address,
                       CpuAccess access = CpuAccess::Sequential);
    void write_word(uint32_t address,
                    uint32_t word,
                    CpuAccess access = CpuAccess::Sequential);

    // not sure what else to do?
    void internal_cycle() { scheduler.add_cycles(1); }
    uint64_t get_cycles() const { return scheduler.get_cycles(); }
    void run(uint64_t);

  private:
    Cpu* cpu;

    template<typename T>
    T read_illegal(uint32_t address) const;

    template<typename T>
    T read(uint32_t address) const;

    template<typename T>
    void write(uint32_t address, T value);

    std::array<CycleCount, 0x10> cycle_map;

    Scheduler scheduler;
    IoDevices io;

    static constexpr uint32_t BOARD_WRAM_SIZE = 1024 * 256;
    static constexpr uint32_t CHIP_WRAM_SIZE  = 1024 * 32;
    static constexpr uint32_t SRAM_SIZE       = 1024 * 256;

    Memory<BIOS_SIZE> bios             = {};
    Memory<BOARD_WRAM_SIZE> board_wram = {};
    Memory<CHIP_WRAM_SIZE> chip_wram   = {};
    Memory<SRAM_SIZE> sram             = {};
    Memory<> rom;

    uint32_t last_bios_word;

    Header header;
    void parse_header();
};
}
