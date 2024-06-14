#pragma once

#include "header.hh"
#include "io/io.hh"
#include <optional>
#include <span>
#include <vector>

namespace matar {
class Bus {
  public:
    static constexpr uint32_t BIOS_SIZE = 1024 * 16;
    Bus(std::array<uint8_t, BIOS_SIZE>&& bios, std::vector<uint8_t>&& rom);

    uint8_t read_byte(uint32_t address);
    void write_byte(uint32_t address, uint8_t byte);

    uint16_t read_halfword(uint32_t address);
    void write_halfword(uint32_t address, uint16_t halfword);

    uint32_t read_word(uint32_t address);
    void write_word(uint32_t address, uint32_t word);

  private:
    template<unsigned int N>
    std::optional<std::span<const uint8_t>> read(uint32_t address) const;

    template<unsigned int N>
    std::optional<std::span<uint8_t>> write(uint32_t address);

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
    std::vector<uint8_t> rom;
    Header header;
    void parse_header();

    IoDevices io;
};
}
