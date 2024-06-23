#include "io/io.hh"
#include "scheduler.hh"
#include "util/log.hh"

namespace matar {

void
IoDevices::scheduler_event(Task::Type type, uint64_t at) {
    switch (type) {
        case Task::Type::DMA0_ACTIVATE: {
            dma.start_transfer(0);
            break;
        }
        case Task::Type::DMA1_ACTIVATE: {
            dma.start_transfer(1);
            break;
        }
        case Task::Type::DMA2_ACTIVATE: {
            dma.start_transfer(2);
            break;
        }
        case Task::Type::DMA3_ACTIVATE: {
            dma.start_transfer(3);
            break;
        }
        case Task::Type::TIMER0_OVERFLOW: {
            timer.trigger_overflow(0, at);
            break;
        }
        case Task::Type::TIMER1_OVERFLOW: {
            timer.trigger_overflow(1, at);
            break;
        }
        case Task::Type::TIMER2_OVERFLOW: {
            timer.trigger_overflow(2, at);
            break;
        }
        case Task::Type::TIMER3_OVERFLOW: {
            timer.trigger_overflow(3, at);
            break;
        }
        case Task::Type::DISPLAY_HDRAW: {
            display.hblank_end(at);
            break;
        }
        case Task::Type::DISPLAY_HBLANK: {
            display.hblank_begin(at);
            break;
        }
        case Task::Type::SAMPLE_PWM: {
            sound.sample(at);
            break;
        }
    }
}

uint8_t
IoDevices::read_byte(uint32_t address) const {
    uint16_t halfword = read_halfword(address & ~1);

    if (address & 1)
        halfword >>= 8;

    return halfword & 0xFF;
}

void
IoDevices::write_byte(uint32_t address, uint8_t byte) {
    uint16_t halfword = read_halfword(address & ~1);

    if (address & 1)
        write_halfword(address & ~1,
                       (static_cast<uint16_t>(byte) << 8) | (halfword & 0xFF));
    else
        write_halfword(address & ~1,
                       (static_cast<uint16_t>(byte) | (halfword & 0xFF00)));
}

uint32_t
IoDevices::read_word(uint32_t address) const {
    return read_halfword(address) | read_halfword(address + 2) << 16;
}

void
IoDevices::write_word(uint32_t address, uint32_t word) {
    write_halfword(address, word & 0xFFFF);
    write_halfword(address + 2, (word >> 16) & 0xFFFF);
}

uint16_t
IoDevices::read_halfword(uint32_t address) const {
    switch (address & 0xFF0) {
        case 0x000:
        case 0x010:
        case 0x020:
        case 0x030:
        case 0x040:
        case 0x050: {
            return display.read_halfword(address);
        }
        case 0x060:
        case 0x070:
        case 0x080:
        case 0x090:
        case 0x0A0: {
            return sound.read_halfword(address);
        }
        case 0x0B0:
        case 0x0C0:
        case 0x0D0:
        case 0x0E0: {
            return dma.read_halfword(address);
        }
        case 0x100: {
            return timer.read_halfword(address);
        }
        case 0x200:
        case 0x300: {
            return system.read_halfword(address);
        }
        default: {
            glogger.debug("Unused I/O address read at 0x{:08X}", address);
        }
    }

    return 0xFF;
}

void
IoDevices::write_halfword(uint32_t address, uint16_t halfword) {
    switch (address & 0xFF0) {
        case 0x000:
        case 0x010:
        case 0x020:
        case 0x030:
        case 0x040:
        case 0x050: {
            display.write_halfword(address, halfword);
            break;
        }
        case 0x060:
        case 0x070:
        case 0x080:
        case 0x090:
        case 0x0A0: {
            sound.write_halfword(address, halfword);
            break;
        }
        case 0x0B0:
        case 0x0C0:
        case 0x0D0:
        case 0x0E0: {
            dma.write_halfword(address, halfword);
            break;
        }
        case 0x100: {
            timer.write_halfword(address, halfword);
            break;
        }
        case 0x200:
        case 0x300: {
            system.write_halfword(address, halfword);
            break;
        }

        default: {
            /*    glogger.debug("Unused I/O address written at 0x{:08X}", address); */
        }
    }
}
}
