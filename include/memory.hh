#pragma once

#include "header.hh"
#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace matar {
class Memory {
  public:
    static constexpr uint32_t BIOS_SIZE = 1024 * 16;

    Memory(std::array<uint8_t, BIOS_SIZE>&& bios, std::vector<uint8_t>&& rom);

    uint8_t read(uint32_t address) const;
    void write(uint32_t address, uint8_t byte);

  private:
#define MEMORY_REGION(name, start)                                             \
    static constexpr uint32_t name##_START = start;

#define DECL_MEMORY(name, ident, start, end)                                   \
    MEMORY_REGION(name, start)                                                 \
    std::array<uint8_t, end - start + 1> ident;

    MEMORY_REGION(BIOS, 0x00000000)
    std::array<uint8_t, BIOS_SIZE> bios;

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

#undef MEMORY_REGION

    std::unordered_map<uint32_t, uint8_t> invalid_mem;
    std::vector<uint8_t> rom;
    Header header;
    void parse_header();
};
}
