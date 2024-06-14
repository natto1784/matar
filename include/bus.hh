#pragma once

#include "header.hh"
#include "io/io.hh"
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace matar {
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

    uint8_t read_byte(uint32_t, bool = true);
    void write_byte(uint32_t, uint8_t, bool = true);

    uint16_t read_halfword(uint32_t, bool = true);
    void write_halfword(uint32_t, uint16_t, bool = true);

    uint32_t read_word(uint32_t, bool = true);
    void write_word(uint32_t, uint32_t, bool = true);

    // not sure what else to do?
    inline void internal_cycle() { cycles++; }

    inline uint32_t get_cycles() { return cycles; }

  private:
    template<unsigned int>
    std::optional<std::span<const uint8_t>> read(uint32_t) const;

    template<unsigned int>
    std::optional<std::span<uint8_t>> write(uint32_t);

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
