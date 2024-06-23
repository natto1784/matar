#include "bus.hh"
#include "cpu/cpu.hh"
#include "io/io.hh"
#include "io/system/registers.hh"
#include "util/crypto.hh"
#include "util/log.hh"
#include <iostream>

namespace matar {

// Constants
static constexpr uint32_t BIOS_START       = 0x0000000;
static constexpr uint32_t BOARD_WRAM_START = 0x2000000;
static constexpr uint32_t CHIP_WRAM_START  = 0x3000000;
static constexpr uint32_t PRAM_START       = 0x5000000;
static constexpr uint32_t VRAM_START       = 0x6000000;
static constexpr uint32_t OAM_START        = 0x7000000;
static constexpr uint32_t ROM_0_START      = 0x8000000;
static constexpr uint32_t ROM_1_START      = 0xA000000;
static constexpr uint32_t ROM_2_START      = 0xC000000;
static constexpr uint32_t IO_START         = 0x4000000;
// static constexpr uint32_t IO_END           = 0x40003FE;
static constexpr uint32_t SRAM_START = 0xE000000;

static constexpr auto
make_cycle_map() {
    std::array<CycleCount, 0x10> map;

    /*
      Region        Bus   Read      Write     Cycles
      BIOS ROM      32    8/16/32   -         1/1/1
      Work RAM 32K  32    8/16/32   8/16/32   1/1/
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

    map.fill({ 1, 1, 1, 1 });

    map[BOARD_WRAM_START >> 24 & 0xF] = CycleCount{ 3, 6, 3, 6 };
    map[OAM_START >> 24 & 0xF]        = CycleCount{ 1, 2, 1, 2 };
    map[PRAM_START >> 24 & 0xF]       = CycleCount{ 1, 2, 1, 2 };
    map[VRAM_START >> 24 & 0xF]       = CycleCount{ 1, 2, 1, 2 };

    return map;
}

Bus::Bus(std::array<uint8_t, BIOS_SIZE>&& bios, std::vector<uint8_t>&& rom)
  : cpu(nullptr)
  , io(*this, scheduler)
  , bios(std::move(bios))
  , rom(std::move(rom)) {
    std::string bios_hash = crypto::sha256(this->bios.data());
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

    cycle_map = make_cycle_map();

    glogger.info("Memory successfully initialised");
    glogger.info("Cartridge Title: {}", header.title);
};

void
Bus::update_cycle_map(WaitstateControl waitcnt) {
    static constexpr std::array<int, 4> WAITSTATE_X_FST = { 4, 3, 2, 8 };
    static constexpr std::array<int, 2> WAITSTATE_0_SND = { 2, 1 };
    static constexpr std::array<int, 2> WAITSTATE_1_SND = { 4, 1 };
    static constexpr std::array<int, 2> WAITSTATE_2_SND = { 8, 1 };

    auto& rom0   = cycle_map[ROM_0_START >> 24 & 0xF];
    auto& rom0_1 = cycle_map[(ROM_0_START >> 24 & 0xF) + 1];
    auto& rom1   = cycle_map[ROM_1_START >> 24 & 0xF];
    auto& rom1_1 = cycle_map[(ROM_1_START >> 24 & 0xF) + 1];
    auto& rom2   = cycle_map[ROM_2_START >> 24 & 0xF];
    auto& rom2_1 = cycle_map[(ROM_2_START >> 24 & 0xF) + 1];
    auto& sram   = cycle_map[SRAM_START >> 24 & 0xF];

    auto reg = waitcnt.value;

    /* SRAM can only be accessed via 8 bit bus */
    sram.n16 = sram.n32 = sram.s16 = sram.s32 =
      WAITSTATE_X_FST[reg.sram_wait_control];

    rom0.n16 = 1 + WAITSTATE_X_FST[reg.wait_state_0_first];
    rom0.s16 = 1 + WAITSTATE_0_SND[reg.wait_state_0_second];
    rom0.n32 = rom0.n16 + rom0.s16;
    rom0.s32 = rom0.s16 + rom0.s16;
    rom0_1   = rom0;

    rom1.n16 = 1 + WAITSTATE_X_FST[reg.wait_state_1_first];
    rom1.s16 = 1 + WAITSTATE_1_SND[reg.wait_state_1_second];
    rom1.n32 = rom1.n16 + rom1.s16;
    rom1.s32 = rom1.s16 + rom1.s16;
    rom1_1   = rom1;

    rom2.n16 = 1 + WAITSTATE_X_FST[reg.wait_state_2_first];
    rom2.s16 = 1 + WAITSTATE_2_SND[reg.wait_state_2_second];
    rom2.n32 = rom2.n16 + rom2.s16;
    rom2.s32 = rom2.s16 + rom2.s16;
    rom2_1   = rom2;
}

void
Bus::step() {
    uint64_t current = get_cycles();

    while (!scheduler.empty() && scheduler.top().cycles <= current) {
        auto event = scheduler.top();
        io.scheduler_event(event.type, event.cycles);
        scheduler.pop();
    }
}

void
Bus::run(uint64_t cyc) {
    while (get_cycles() < cyc) {
        if (!scheduler.empty()) {
            // glogger.info("cycling for {} cycles",
            // scheduler.top().cycles - get_cycles());

            while (get_cycles() < scheduler.top().cycles) {
                if (io.any_is_interrupt_pending()) {
                    cpu->irq();
                }

                cpu->step();
            }

            while (!scheduler.empty() &&
                   scheduler.top().cycles <= get_cycles()) {
                auto event = scheduler.top();
                io.scheduler_event(event.type, event.cycles);
                scheduler.pop();
            }
        } else {
            if (io.any_is_interrupt_pending()) {
                cpu->irq();
            }

            cpu->step();
        }
    }
}

template<typename T>
T
Bus::read_illegal(uint32_t address) const {
    uint32_t value;
    uint32_t decoded;
    uint32_t prefetched;
    uint32_t pc;

    if (cpu == nullptr) {
        glogger.error("cpu is null, make sure to assign it to the Bus");
        std::abort();
    }

    decoded    = cpu->opcode0() & 0xFFFF;
    prefetched = cpu->opcode1() & 0xFFFF;
    pc         = cpu->program_counter();

    if (cpu->state() == State::Arm) {
        return static_cast<T>(cpu->opcode1());
    } else {
        switch (pc >> 24 & 0xF) {
            case BIOS_START >> 24 & 0xF:
            case OAM_START >> 24 & 0xF: {
                value = (prefetched << 16 | decoded);
                break;
            }
            case CHIP_WRAM_START >> 24 & 0xF: {
                if (pc & 0b11) {
                    /* unaligned */
                    value = (prefetched << 16 | decoded);
                } else {
                    /* aligned */
                    value = (decoded << 16 | prefetched);
                }
                break;
            }
            default: {
                value = (prefetched << 16 | prefetched);
            }
        }
    }

    return static_cast<T>(value >> ((address & 0b11) << 3));
}

uint8_t
Bus::read_byte(uint32_t address, CpuAccess access) {
    auto cc = cycle_map[(address >> 24) & 0xF];
    scheduler.add_cycles(access == CpuAccess::Sequential ? cc.s16 : cc.n16);

    switch ((address >> 24) & 0xF) {
        case (BIOS_START >> 24) & 0xF: {
            uint32_t offset = address - BIOS_START;
            if (offset >= bios.size()) {
                return read_illegal<uint8_t>(address);
            } else if (cpu->program_counter() >= bios.size()) {
                return last_bios_word;
            }

            last_bios_word = bios.read_byte(offset);
            return last_bios_word;
        }

        case (BOARD_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (board_wram.size() - 1);

            return board_wram.read_byte(offset);
        }

        case (CHIP_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (chip_wram.size() - 1);

            return chip_wram.read_byte(offset);
        }

        case (IO_START >> 24) & 0xF: {
            if ((address & 0xff0800) != 0) {
                address &= ~0xff0000;
            }
            return io.read_byte(address);
        }

        case (PRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.pram().size() - 1);

            return io.pram().read_byte(offset);
        }

        case (VRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (128 * 1024 - 1);

            if (offset >= 96 * 1024) {
                offset -= 32 * 1024;
            }

            return io.vram().read_byte(offset);
        }

        case (OAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.oam().size() - 1);

            return io.oam().read_byte(offset);
        }

        case (ROM_0_START >> 24) & 0xF:
        case ((ROM_0_START >> 24) & 0xF) + 1:
        case (ROM_1_START >> 24) & 0xF:
        case ((ROM_1_START >> 24) & 0xF) + 1:
        case (ROM_2_START >> 24) & 0xF:
        case ((ROM_2_START >> 24) & 0xF) + 1: {
            uint32_t offset = address & (32 * 1024 * 1024 - 1);

            if (offset >= rom.size()) {
                glogger.error("invalid ROM region read at {:08x}", address);
                return read_illegal<uint8_t>(address);
            }

            return rom.read_byte(offset);
        }

        default:
            return read_illegal<uint8_t>(address);
    }
}

uint16_t
Bus::read_halfword(uint32_t address, CpuAccess access) {
    auto cc = cycle_map[(address >> 24) & 0xF];
    scheduler.add_cycles(access == CpuAccess::Sequential ? cc.s16 : cc.n16);

    switch ((address >> 24) & 0xF) {
        case (BIOS_START >> 24) & 0xF: {
            uint32_t offset = address - BIOS_START;
            if (offset >= bios.size()) {
                return read_illegal<uint8_t>(address);
            } else if (cpu->program_counter() >= bios.size()) {
                return last_bios_word;
            }

            last_bios_word = bios.read_halfword(offset);
            return last_bios_word;
        }

        case (BOARD_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (board_wram.size() - 1);

            return board_wram.read_halfword(offset);
        }

        case (CHIP_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (chip_wram.size() - 1);

            return chip_wram.read_halfword(offset);
        }

        case (IO_START >> 24) & 0xF: {
            if ((address & 0xff0800) != 0) {
                address &= ~0xff0000;
            }
            return io.read_halfword(address);
        }

        case (PRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.pram().size() - 1);

            return io.pram().read_halfword(offset);
        }

        case (VRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (128 * 1024 - 1);

            if (offset >= 96 * 1024) {
                offset -= 32 * 1024;
            }

            return io.vram().read_halfword(offset);
        }

        case (OAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.oam().size() - 1);

            return io.oam().read_halfword(offset);
        }

        case (ROM_0_START >> 24) & 0xF:
        case ((ROM_0_START >> 24) & 0xF) + 1:
        case (ROM_1_START >> 24) & 0xF:
        case ((ROM_1_START >> 24) & 0xF) + 1:
        case (ROM_2_START >> 24) & 0xF:
        case ((ROM_2_START >> 24) & 0xF) + 1: {
            uint32_t offset = address & (32 * 1024 * 1024 - 1);

            if (offset >= rom.size()) {
                glogger.error("invalid ROM region read at {:08x}", address);
                return read_illegal<uint8_t>(address);
            }

            return rom.read_halfword(offset);
        }

        default:
            return read_illegal<uint8_t>(address);
    }
}

uint32_t
Bus::read_word(uint32_t address, CpuAccess access) {
    auto cc = cycle_map[(address >> 24) & 0xF];
    scheduler.add_cycles(access == CpuAccess::Sequential ? cc.s32 : cc.n32);

    switch ((address >> 24) & 0xF) {
        case (BIOS_START >> 24) & 0xF: {
            uint32_t offset = address - BIOS_START;
            if (offset >= bios.size()) {
                return read_illegal<uint8_t>(address);
            } else if (cpu->program_counter() >= bios.size()) {
                return last_bios_word;
            }

            last_bios_word = bios.read_word(offset);
            return last_bios_word;
        }

        case (BOARD_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (board_wram.size() - 1);

            return board_wram.read_word(offset);
        }

        case (CHIP_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (chip_wram.size() - 1);

            return chip_wram.read_word(offset);
        }

        case (IO_START >> 24) & 0xF: {
            if ((address & 0xff0800) != 0) {
                address &= ~0xff0000;
            }
            return io.read_word(address);
        }

        case (PRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.pram().size() - 1);

            return io.pram().read_word(offset);
        }

        case (VRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (128 * 1024 - 1);

            if (offset >= 96 * 1024) {
                offset -= 32 * 1024;
            }

            return io.vram().read_word(offset);
        }

        case (OAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.oam().size() - 1);

            return io.oam().read_word(offset);
        }

        case (ROM_0_START >> 24) & 0xF:
        case ((ROM_0_START >> 24) & 0xF) + 1:
        case (ROM_1_START >> 24) & 0xF:
        case ((ROM_1_START >> 24) & 0xF) + 1:
        case (ROM_2_START >> 24) & 0xF:
        case ((ROM_2_START >> 24) & 0xF) + 1: {
            uint32_t offset = address & (32 * 1024 * 1024 - 1);

            if (offset >= rom.size()) {
                glogger.error("invalid ROM region read at {:08x}", address);
                return read_illegal<uint8_t>(address);
            }

            return rom.read_word(offset);
        }

        default:
            return read_illegal<uint8_t>(address);
    }
}

void
Bus::write_byte(uint32_t address, uint8_t byte, CpuAccess access) {
    auto cc = cycle_map[(address >> 24) & 0xF];
    scheduler.add_cycles(access == CpuAccess::Sequential ? cc.s16 : cc.n16);

    switch ((address >> 24) & 0xF) {
        case (BOARD_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (board_wram.size() - 1);

            board_wram.write_byte(offset, byte);
            break;
        }

        case (CHIP_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (chip_wram.size() - 1);

            chip_wram.write_byte(offset, byte);
            break;
        }

        case (IO_START >> 24) & 0xF: {
            if ((address & 0xff0800) != 0) {
                address &= ~0xff0000;
            }

            io.write_byte(address, byte);
            break;
        }

        case (VRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (128 * 1024 - 1);

            if (offset >= 96 * 1024) {
                offset -= 32 * 1024;
            }

            if (offset >= io.obj_offset()) {
                break;
            }

            io.vram().write_halfword(offset & ~1,
                                     static_cast<uint16_t>(byte) * 0x101);
            break;
        }

        case (PRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.pram().size() - 1);

            io.pram().write_halfword(offset & ~1,
                                     static_cast<uint16_t>(byte) * 0x101);
            break;
        }

        default:
            glogger.error("invalid write {:08x} : {:02x}", address, byte);
    }
}

void
Bus::write_halfword(uint32_t address, uint16_t halfword, CpuAccess access) {
    auto cc = cycle_map[(address >> 24) & 0xF];
    scheduler.add_cycles(access == CpuAccess::Sequential ? cc.s16 : cc.n16);

    switch ((address >> 24) & 0xF) {
        case (BOARD_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (board_wram.size() - 1);

            board_wram.write_halfword(offset, halfword);
            break;
        }

        case (CHIP_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (chip_wram.size() - 1);

            chip_wram.write_halfword(offset, halfword);
            break;
        }

        case (IO_START >> 24) & 0xF: {
            if ((address & 0xff0800) != 0) {
                address &= ~0xff0000;
            }

            io.write_halfword(address, halfword);
            break;
        }

        case (PRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.pram().size() - 1);

            io.pram().write_halfword(offset, halfword);
            break;
        }

        case (VRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (128 * 1024 - 1);

            if (offset >= 96 * 1024) {
                offset -= 32 * 1024;
            }

            io.vram().write_halfword(offset, halfword);
            break;
        }

        case (OAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.oam().size() - 1);

            io.oam().write_halfword(offset, halfword);
            break;
        }

        case (ROM_0_START >> 24) & 0xF:
        case ((ROM_0_START >> 24) & 0xF) + 1:
        case (ROM_1_START >> 24) & 0xF:
        case ((ROM_1_START >> 24) & 0xF) + 1:
        case (ROM_2_START >> 24) & 0xF:
        case ((ROM_2_START >> 24) & 0xF) + 1: {
            uint32_t offset = address & (32 * 1024 * 1024 - 1);

            if (offset >= rom.size()) {
                glogger.error("invalid ROM region written at {:08x}", address);
            }

            rom.write_halfword(offset, halfword);
            break;
        }

        default:
            glogger.error("invalid write {:08x} : {:04x}", address, halfword);
    }
}

void
Bus::write_word(uint32_t address, uint32_t word, CpuAccess access) {
    auto cc = cycle_map[(address >> 24) & 0xF];
    scheduler.add_cycles(access == CpuAccess::Sequential ? cc.s32 : cc.n32);

    switch ((address >> 24) & 0xF) {
        case (BOARD_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (board_wram.size() - 1);

            board_wram.write_word(offset, word);
            break;
        }

        case (CHIP_WRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (chip_wram.size() - 1);

            chip_wram.write_word(offset, word);
            break;
        }

        case (IO_START >> 24) & 0xF: {
            if ((address & 0x800) == 0x800) {
                address &= ~0xff0000;
            }

            io.write_word(address, word);
            break;
        }

        case (PRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.pram().size() - 1);

            io.pram().write_word(offset, word);
            break;
        }

        case (VRAM_START >> 24) & 0xF: {
            uint32_t offset = address & (128 * 1024 - 1);

            if (offset >= 96 * 1024) {
                offset -= 32 * 1024;
            }

            io.vram().write_word(offset, word);
            break;
        }

        case (OAM_START >> 24) & 0xF: {
            uint32_t offset = address & (io.oam().size() - 1);

            io.oam().write_word(offset, word);
            break;
        }

        case (ROM_0_START >> 24) & 0xF:
        case ((ROM_0_START >> 24) & 0xF) + 1:
        case (ROM_1_START >> 24) & 0xF:
        case ((ROM_1_START >> 24) & 0xF) + 1:
        case (ROM_2_START >> 24) & 0xF:
        case ((ROM_2_START >> 24) & 0xF) + 1: {
            uint32_t offset = address & (32 * 1024 * 1024 - 1);

            if (offset >= rom.size()) {
                glogger.error("invalid ROM region written at {:08x}", address);
            }

            rom.write_word(offset, word);
            break;
        }

        default:
            glogger.error("invalid write {:08x} : {:04x}", address, word);
    }
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
