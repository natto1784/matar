#include "memory.hh"
#include "header.hh"
#include "util/bits.hh"
#include "util/crypto.hh"
#include "util/log.hh"
#include <bitset>
#include <stdexcept>

namespace matar {
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
        glogger.warn("BIOS hash failed to match, run at your own risk"
                     "\nExpected : {} "
                     "\nGot      : {}",
                     expected_hash,
                     bios_hash);
    }

    parse_header();

    glogger.info("Memory successfully initialised");
    glogger.info("Cartridge Title: {}", header.title);
};

uint8_t
Memory::read(size_t address) const {
#define MATCHES(AREA, area)                                                    \
    if (address >= AREA##_START && address < AREA##_START + area.size())       \
        return area[address - AREA##_START];

    MATCHES(BIOS, bios)
    MATCHES(BOARD_WRAM, board_wram)
    MATCHES(CHIP_WRAM, chip_wram)
    MATCHES(PALETTE_RAM, palette_ram)
    MATCHES(VRAM, vram)
    MATCHES(OAM_OBJ_ATTR, oam_obj_attr)
    MATCHES(ROM_0, rom)
    MATCHES(ROM_1, rom)
    MATCHES(ROM_2, rom)

    glogger.error("Invalid memory region accessed");
    return 0xFF;

#undef MATCHES
}

void
Memory::write(size_t address, uint8_t byte) {
#define MATCHES(AREA, area)                                                    \
    if (address >= AREA##_START && address < AREA##_START + area.size()) {     \
        area[address - AREA##_START] = byte;                                   \
        return;                                                                \
    }

    MATCHES(BOARD_WRAM, board_wram)
    MATCHES(CHIP_WRAM, chip_wram)
    MATCHES(PALETTE_RAM, palette_ram)
    MATCHES(VRAM, vram)
    MATCHES(OAM_OBJ_ATTR, oam_obj_attr)

    glogger.error("Invalid memory region accessed");

#undef MATCHES
}

void
Memory::parse_header() {
    if (rom.size() < header.HEADER_SIZE) {
        throw std::out_of_range(
          "ROM is not large enough to even have a header");
    }

    // entrypoint
    header.entrypoint =
      rom[0x00] | rom[0x01] << 8 | rom[0x02] << 16 | rom[0x03] << 24;

    // nintendo logo
    if (rom[0x9C] != 0x21)
        glogger.info("HEADER: BIOS debugger bits not set to 0");

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
            glogger.error("HEADER: invalid unique code: {}", rom[0xAC]);
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
            glogger.error("HEADER: invalid destination/language: {}",
                          rom[0xAF]);
    }

    if (rom[0xB2] != 0x96)
        glogger.error("HEADER: invalid fixed byte at 0xB2");

    for (size_t i = 0xB5; i < 0xBC; i++) {
        if (rom[i] != 0x00)
            glogger.error("HEADER: invalid fixed bytes at 0xB5");
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
            glogger.error("HEADER: checksum does not match");
    }

    // multiboot not required right now
}
}
