#pragma once

#include "header.hh"
#include "io/io.hh"
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace matar {
enum CpuAccess {
    Sequential,
    NonSequential
};

enum CpuAccessWidth {
    Word,
    Halfword,
    Byte
};

class Bus {
  private:
    struct Private {
        explicit Private() = default;
    };

  public:
    static constexpr uint32_t BIOS_SIZE = 1024 * 16;

    Bus(Private, std::array<uint8_t, BIOS_SIZE>&&, std::vector<uint8_t>&&);

    static std::shared_ptr<Bus> init(std::array<uint8_t, BIOS_SIZE>&&,
                                     std::vector<uint8_t>&&);

    uint8_t read_byte(uint32_t address, CpuAccess access) {
        add_cpu_cycles<CpuAccessWidth::Byte>(address, access);
        return read_byte(address);
    };
    void write_byte(uint32_t address, uint8_t byte, CpuAccess access) {
        add_cpu_cycles<CpuAccessWidth::Byte>(address, access);
        write_byte(address, byte);
    };

    uint16_t read_halfword(uint32_t address, CpuAccess access) {
        add_cpu_cycles<CpuAccessWidth::Halfword>(address, access);
        return read_halfword(address);
    }
    void write_halfword(uint32_t address, uint16_t halfword, CpuAccess access) {
        add_cpu_cycles<CpuAccessWidth::Halfword>(address, access);
        write_halfword(address, halfword);
    }

    uint32_t read_word(uint32_t address, CpuAccess access) {
        add_cpu_cycles<CpuAccessWidth::Word>(address, access);
        return read_word(address);
    }
    void write_word(uint32_t address, uint32_t word, CpuAccess access) {
        add_cpu_cycles<CpuAccessWidth::Word>(address, access);
        write_word(address, word);
    }

    uint8_t read_byte(uint32_t address);
    void write_byte(uint32_t address, uint8_t byte);

    uint16_t read_halfword(uint32_t address);
    void write_halfword(uint32_t address, uint16_t halfword);

    uint32_t read_word(uint32_t address);
    void write_word(uint32_t address, uint32_t word);

    // not sure what else to do?
    void internal_cycle() { cycles++; }
    uint32_t get_cycles() { return cycles; }

  private:
    template<CpuAccessWidth W>
    void add_cpu_cycles(uint32_t address, CpuAccess access) {
        auto cc = cycle_map[address >> 24 & 0xF];
        if constexpr (W == CpuAccessWidth::Word) {
            cycles += (access == CpuAccess::Sequential ? cc.s32 : cc.n32);
        } else {
            cycles += (access == CpuAccess::Sequential ? cc.s16 : cc.n16);
        }
    }

    template<typename T>
    std::optional<T> read(uint32_t address) const;

    template<typename T>
    void write(uint32_t address, T value);

    uint32_t cycles = 0;
    struct cycle_count {
        uint8_t n16; // non sequential 8/16 bit width access
        uint8_t n32; // non sequential 32 bit width access
        uint8_t s16; // seuquential 8/16 bit width access
        uint8_t s32; // sequential 32 bit width access
    };
    std::array<cycle_count, 0x10> cycle_map;
    static constexpr decltype(cycle_map) init_cycle_count();

    std::unique_ptr<IoDevices> io;

#define MEMORY_REGION(name, start)                                             \
    static constexpr uint32_t name##_START = start;                            \
    static constexpr uint8_t name##_REGION = start >> 24 & 0xF;

#define DECL_MEMORY(name, ident, start, end)                                   \
    MEMORY_REGION(name, start)                                                 \
    std::array<uint8_t, end - start + 1> ident = {};

    MEMORY_REGION(BIOS, 0x00000000)
    std::array<uint8_t, BIOS_SIZE> bios = {};

    // board working RAM
    DECL_MEMORY(BOARD_WRAM, board_wram, 0x02000000, 0x0203FFFF)

    // chip working RAM
    DECL_MEMORY(CHIP_WRAM, chip_wram, 0x03000000, 0x03007FFF)

    // palette RAM
    DECL_MEMORY(PALETTE_RAM, palette_ram, 0x05000000, 0x050003FF)

    // video RAM
    DECL_MEMORY(VRAM, vram, 0x06000000, 0x06017FFF)

    // OAM OBJ attributes
    DECL_MEMORY(OAM_OBJ_ATTR, oam_obj_attr, 0x07000000, 0x070003FF)

#undef DECL_MEMORY

    MEMORY_REGION(ROM_0, 0x08000000)
    MEMORY_REGION(ROM_1, 0x0A000000)
    MEMORY_REGION(ROM_2, 0x0C000000)

    MEMORY_REGION(IO, 0x04000000)
    static constexpr uint32_t IO_END = 0x040003FE;

#undef MEMORY_REGION

    std::vector<uint8_t> rom;
    Header header;
    void parse_header();
};
}
