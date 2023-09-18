#include "memory.hh"
#include "header.hh"
#include "util/bits.hh"
#include "util/log.hh"
#include "util/utils.hh"
#include <bitset>
#include <stdexcept>

using namespace logger;

Memory::Memory(std::array<uint8_t, BIOS_SIZE>&& bios,
               std::vector<uint8_t>&& rom)
  : bios(std::move(bios))
  , board_wram({ 0 })
  , chip_wram({ 0 })
  , palette_ram({ 0 })
  , vram({ 0 })
  , oam_obj_attr({ 0 })
  , rom(std::move(rom)) {
    std::string bios_hash = crypto::sha256(this->bios);
    static constexpr std::string_view expected_hash =
      "fd2547724b505f487e6dcb29ec2ecff3af35a841a77ab2e85fd87350abd36570";

    if (bios_hash != expected_hash) {
        log_warn("BIOS hash failed to match, run at your own risk"
                 "\nExpected : {} "
                 "\nGot      : {}",
                 expected_hash,
                 bios_hash);
    }

    parse_header();

    log_info("Memory successfully initialised");
    log_info("Cartridge Title: {}", header.title);
};

#define MATCHES(area) address >= area##_START&& address <= area##_END

uint8_t
Memory::read(size_t address) const {
    if (MATCHES(BIOS)) {
        return bios[address];
    } else if (MATCHES(BOARD_WRAM)) {
        return board_wram[address - BOARD_WRAM_START];
    } else if (MATCHES(CHIP_WRAM)) {
        return chip_wram[address - CHIP_WRAM_START];
    } else if (MATCHES(PALETTE_RAM)) {
        return palette_ram[address - PALETTE_RAM_START];
    } else if (MATCHES(VRAM)) {
        return vram[address - VRAM_START];
    } else if (MATCHES(OAM_OBJ_ATTR)) {
        return oam_obj_attr[address - OAM_OBJ_ATTR_START];
    } else if (MATCHES(ROM_0)) {
        return rom[address - ROM_0_START];
    } else if (MATCHES(ROM_1)) {
        return rom[address - ROM_1_START];
    } else if (MATCHES(ROM_2)) {
        return rom[address - ROM_2_START];
    } else {
        log_error("Invalid memory region accessed");
        return 0xFF;
    }
}

void
Memory::write(size_t address, uint8_t byte) {
    if (MATCHES(BIOS)) {
        bios[address] = byte;
    } else if (MATCHES(BOARD_WRAM)) {
        board_wram[address - BOARD_WRAM_START] = byte;
    } else if (MATCHES(CHIP_WRAM)) {
        chip_wram[address - CHIP_WRAM_START] = byte;
    } else if (MATCHES(PALETTE_RAM)) {
        palette_ram[address - PALETTE_RAM_START] = byte;
    } else if (MATCHES(VRAM)) {
        vram[address - VRAM_START] = byte;
    } else if (MATCHES(OAM_OBJ_ATTR)) {
        oam_obj_attr[address - OAM_OBJ_ATTR_START] = byte;
    } else if (MATCHES(ROM_0)) {
        rom[address - ROM_0_START] = byte;
    } else if (MATCHES(ROM_1)) {
        rom[address - ROM_1_START] = byte;
    } else if (MATCHES(ROM_2)) {
        rom[address - ROM_2_START] = byte;
    } else {
        log_error("Invalid memory region accessed");
    }
}

#undef MATCHES

uint16_t
Memory::read_halfword(size_t address) const {
    if (address & 0b01)
        log_warn("Reading a non aligned halfword address");

    return read(address) | read(address + 1) << 8;
}

void
Memory::write_halfword(size_t address, uint16_t halfword) {
    if (address & 0b01)
        log_warn("Writing to a non aligned halfword address");

    write(address, halfword & 0xFF);
    write(address + 1, halfword >> 8 & 0xFF);
}

uint32_t
Memory::read_word(size_t address) const {
    if (address & 0b11)
        log_warn("Reading a non aligned word address");

    return read(address) | read(address + 1) << 8 | read(address + 2) << 16 |
           read(address + 3) << 24;
}

void
Memory::write_word(size_t address, uint32_t word) {
    if (address & 0b11)
        log_warn("Writing to a non aligned word address");

    write(address, word & 0xFF);
    write(address + 1, word >> 8 & 0xFF);
    write(address + 2, word >> 16 & 0xFF);
    write(address + 3, word >> 24 & 0xFF);
}

void
Memory::parse_header() {

    if (rom.size() < 192) {
        throw std::out_of_range(
          "ROM is not large enough to even have a header");
    }

    // entrypoint
    header.entrypoint =
      rom[0x00] | rom[0x01] << 8 | rom[0x02] << 16 | rom[0x03] << 24;

    // nintendo logo
    if (rom[0x9C] != 0x21)
        log_info("HEADER: BIOS debugger bits not set to 0");

    // game info
    header.title = std::string(&rom[0xA0], &rom[0xA0 + 12]);

    switch (rom[0xAC]) {
        case 'A':
            header.unique_code = Header::UniqueCode::Old;
            break;
        case 'B':
            header.unique_code = Header::UniqueCode::New;
            break;
        case 'C':
            header.unique_code = Header::UniqueCode::Newer;
            break;
        case 'F':
            header.unique_code = Header::UniqueCode::Famicom;
            break;
        case 'K':
            header.unique_code = Header::UniqueCode::YoshiKoro;
            break;
        case 'P':
            header.unique_code = Header::UniqueCode::Ereader;
            break;
        case 'R':
            header.unique_code = Header::UniqueCode::Warioware;
            break;
        case 'U':
            header.unique_code = Header::UniqueCode::Boktai;
            break;
        case 'V':
            header.unique_code = Header::UniqueCode::DrillDozer;
            break;

        default:
            log_error("HEADER: invalid unique code: {}", rom[0xAC]);
    }

    header.title_code = std::string(&rom[0xAD], &rom[0xAE]);

    switch (rom[0xAF]) {
        case 'J':
            header.i18n = Header::I18n::Japan;
            break;
        case 'P':
            header.i18n = Header::I18n::Europe;
            break;
        case 'F':
            header.i18n = Header::I18n::French;
            break;
        case 'S':
            header.i18n = Header::I18n::Spanish;
            break;
        case 'E':
            header.i18n = Header::I18n::Usa;
            break;
        case 'D':
            header.i18n = Header::I18n::German;
            break;
        case 'I':
            header.i18n = Header::I18n::Italian;
            break;

        default:
            log_error("HEADER: invalid destination/language: {}", rom[0xAF]);
    }

    if (rom[0xB2] != 0x96)
        log_error("HEADER: invalid fixed byte at 0xB2");

    for (size_t i = 0xB5; i < 0xBC; i++) {
        if (rom[i] != 0x00)
            log_error("HEADER: invalid fixed bytes at 0xB5");
    }

    header.version = rom[0xBC];

    // checksum
    {
        size_t i = 0xA0, chk = 0;
        while (i <= 0xBC)
            chk -= rom[i++];
        chk -= 0x19;
        chk &= 0xFF;

        if (chk != rom[0xBD])
            log_error("HEADER: checksum does not match");
    }

    // multiboot not required right now
}
