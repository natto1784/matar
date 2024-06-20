#include "bus.hh"
#include "io/io.hh"
#include "util/crypto.hh"
#include "util/log.hh"

namespace matar {

Bus::Bus(Private,
         std::array<uint8_t, BIOS_SIZE>&& bios,
         std::vector<uint8_t>&& rom)
  : cycle_map(init_cycle_count())
  , bios(std::move(bios))
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

std::shared_ptr<Bus>
Bus::init(std::array<uint8_t, BIOS_SIZE>&& bios, std::vector<uint8_t>&& rom) {
    auto self =
      std::make_shared<Bus>(Private(), std::move(bios), std::move(rom));
    self->io = std::make_unique<IoDevices>(self);
    return self;
}

constexpr decltype(Bus::cycle_map)
Bus::init_cycle_count() {
    /*
      Region        Bus   Read      Write     Cycles
      BIOS ROM      32    8/16/32   -         1/1/1
      Work RAM 32K  32    8/16/32   8/16/32   1/1/1
      I/O           32    8/16/32   8/16/32   1/1/1
      OAM           32    8/16/32   16/32     1/1/1 *
      Work RAM 256K 16    8/16/32   8/16/32   3/3/6 **
      Palette RAM   16    8/16/32   16/32     1/1/2 *
      VRAM          16    8/16/32   16/32     1/1/2 *
      GamePak ROM   16    8/16/32   -         5/5/8 **|***
      GamePak Flash 16    8/16/32   16/32     5/5/8 **|***
      GamePak SRAM  8     8         8         5     **

    Timing Notes:

      *   Plus 1 cycle if GBA accesses video memory at the same time.
      **  Default waitstate settings, see System Control chapter.
      *** Separate timings for sequential, and non-sequential accesses.
      One cycle equals approx. 59.59ns (ie. 16.78MHz clock).
    */

    decltype(cycle_map) map;
    map.fill({ 1, 1, 1, 1 });

    /* used fill instead of this
    map[BIOS_REGION]      = { 1, 1, 1, 1 };
    map[CHIP_WRAM_REGION] = { 1, 1, 1, 1 };
    map[IO_REGION]        = { 1, 1, 1, 1 };
    map[OAM_REGION]       = { 1, 1, 1, 1 };
    */
    map[3]                  = { 1, 1, 1, 1 };
    map[BOARD_WRAM_REGION]  = { .n16 = 3, .n32 = 6, .s16 = 3, .s32 = 6 };
    map[PALETTE_RAM_REGION] = { .n16 = 1, .n32 = 2, .s16 = 1, .s32 = 2 };
    map[VRAM_REGION]        = { .n16 = 1, .n32 = 2, .s16 = 1, .s32 = 2 };
    // TODO: GamePak access cycles

    return map;
}

template<typename T>
std::optional<T>
Bus::read(uint32_t address) const {

    // this is cleaned than std::enable_if
    static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
                    std::is_same_v<T, uint32_t>,
                  "Can only read uint8_t, uin16_t or uint32_t");

    constexpr int N = std::is_same_v<T, uint8_t>    ? 1
                      : std::is_same_v<T, uint16_t> ? 2
                      : std::is_same_v<T, uint32_t> ? 4
                                                    : 0;

    switch (address >> 24 & 0xF) {
#define MATCHES(AREA, area)                                                    \
    case AREA##_REGION: {                                                      \
        uint32_t i = address - AREA##_START;                                   \
        if (i > area.size() - N)                                               \
            break;                                                             \
        if constexpr (std::is_same_v<T, uint8_t>)                              \
            return area[i];                                                    \
        else if constexpr (std::is_same_v<T, uint16_t>)                        \
            return area[i] | area[i + 1] << 8;                                 \
        else if constexpr (std::is_same_v<T, uint32_t>)                        \
            return area[i] | area[i + 1] << 8 | area[i + 2] << 16 |            \
                   area[i + 3] << 24;                                          \
    }

#define MATCHES_PAK(AREA, area)                                                \
    case AREA##_REGION + 1:                                                    \
        MATCHES(AREA, area)

        MATCHES(BIOS, bios)
        MATCHES(BOARD_WRAM, board_wram)
        MATCHES(CHIP_WRAM, chip_wram)
        MATCHES(PALETTE_RAM, palette_ram)
        MATCHES(VRAM, vram)
        MATCHES(OAM_OBJ_ATTR, oam_obj_attr)

        MATCHES_PAK(ROM_0, rom)
        MATCHES_PAK(ROM_1, rom)
        MATCHES_PAK(ROM_2, rom)
#undef MATCHES_PAK
#undef MATCHES
    }

    glogger.error("Invalid memory region read");
    return {};
}

template<typename T>
void
Bus::write(uint32_t address, T value) {
    static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t> ||
                    std::is_same_v<T, uint32_t>,
                  "Can only write uint8_t, uin16_t or uint32_t");

    constexpr int N = std::is_same_v<T, uint8_t>    ? 1
                      : std::is_same_v<T, uint16_t> ? 2
                      : std::is_same_v<T, uint32_t> ? 4
                                                    : 0;
    switch (address >> 24 & 0xF) {
#define MATCHES(AREA, area)                                                    \
    case AREA##_REGION: {                                                      \
        uint32_t i = address - AREA##_START;                                   \
        if (i > area.size() - N)                                               \
            break;                                                             \
        if constexpr (std::is_same_v<T, uint8_t>) {                            \
            area[i] = value;                                                   \
        } else if constexpr (std::is_same_v<T, uint16_t>) {                    \
            area[i]     = value & 0xFF;                                        \
            area[i + 1] = value >> 8 & 0xFF;                                   \
        } else if constexpr (std::is_same_v<T, uint32_t>) {                    \
            area[i]     = value & 0xFF;                                        \
            area[i + 1] = value >> 8 & 0xFF;                                   \
            area[i + 2] = value >> 16 & 0xFF;                                  \
            area[i + 3] = value >> 24 & 0xFF;                                  \
        }                                                                      \
        return;                                                                \
    }

        MATCHES(BOARD_WRAM, board_wram)
        MATCHES(CHIP_WRAM, chip_wram)
        MATCHES(PALETTE_RAM, palette_ram)
        MATCHES(VRAM, vram)
        MATCHES(OAM_OBJ_ATTR, oam_obj_attr)

#undef MATCHES
    }

    glogger.error("Invalid memory region written");
}

uint8_t
Bus::read_byte(uint32_t address) {
    if (address >= IO_START && address <= IO_END)
        return io->read_byte(address);

    return read<uint8_t>(address).value_or(0xFF);
}

void
Bus::write_byte(uint32_t address, uint8_t byte) {
    if (address >= IO_START && address <= IO_END) {
        io->write_byte(address, byte);
        return;
    }

    write<uint8_t>(address, byte);
}

uint16_t
Bus::read_halfword(uint32_t address) {
    if (address & 0b01)
        glogger.warn("Reading a non aligned halfword address");

    if (address >= IO_START && address <= IO_END)
        return io->read_halfword(address);

    return read<uint16_t>(address).value_or(0xFFFF);
}

void
Bus::write_halfword(uint32_t address, uint16_t halfword) {
    if (address & 0b01)
        glogger.warn("Writing to a non aligned halfword address");

    if (address >= IO_START && address <= IO_END) {
        io->write_halfword(address, halfword);
        return;
    }

    write<uint16_t>(address, halfword);
}

uint32_t
Bus::read_word(uint32_t address) {
    if (address & 0b11)
        glogger.warn("Reading a non aligned word address");

    if (address >= IO_START && address <= IO_END)
        return io->read_word(address);

    return read<uint32_t>(address).value_or(0xFFFFFFFF);
}

void
Bus::write_word(uint32_t address, uint32_t word) {
    if (address & 0b11)
        glogger.warn("Writing to a non aligned word address");

    if (address >= IO_START && address <= IO_END) {
        io->write_word(address, word);
        return;
    }

    write<uint32_t>(address, word);
}

void
Bus::parse_header() {
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

    for (uint32_t i = 0xB5; i < 0xBC; i++) {
        if (rom[i] != 0x00)
            glogger.error("HEADER: invalid fixed bytes at 0xB5");
    }

    header.version = rom[0xBC];

    // checksum
    {
        uint32_t i = 0xA0, chk = 0;
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
