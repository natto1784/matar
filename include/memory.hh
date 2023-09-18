#pragma once

#include "header.hh"
#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

class Memory {
  public:
    static constexpr size_t BIOS_SIZE = 1024 * 16;

    Memory(std::array<uint8_t, BIOS_SIZE>&& bios, std::vector<uint8_t>&& rom);

    uint8_t read(size_t address) const;
    void write(size_t address, uint8_t byte);

    uint16_t read_halfword(size_t address) const;
    void write_halfword(size_t address, uint16_t halfword);

    uint32_t read_word(size_t address) const;
    void write_word(size_t address, uint32_t word);

  private:
#define MEMORY_REGION(name, start, end)                                        \
    static constexpr size_t name##_START = start;                              \
    static constexpr size_t name##_END   = end;

#define DECL_MEMORY(name, ident, start, end)                                   \
    MEMORY_REGION(name, start, end)                                            \
    std::array<uint8_t, name##_END - name##_START + 1> ident;

    MEMORY_REGION(BIOS, 0x00000000, 0x00003FFF)
    std::array<uint8_t, BIOS_SIZE> bios;
    static_assert(BIOS_END - BIOS_START + 1 == BIOS_SIZE);

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

    MEMORY_REGION(ROM_0, 0x08000000, 0x09FFFFFF)
    MEMORY_REGION(ROM_1, 0x0A000000, 0x0BFFFFFF)
    MEMORY_REGION(ROM_2, 0x0C000000, 0x0DFFFFFF)

#undef MEMORY_REGION

    std::unordered_map<size_t, uint8_t> invalid_mem;
    std::vector<uint8_t> rom;
    Header header;
    void parse_header();
};
