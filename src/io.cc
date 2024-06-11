#include "io.hh"
#include "util/log.hh"

namespace matar {
#define ADDR static constexpr uint32_t

ADDR POSTFLG = 0x4000300;
ADDR IME     = 0x4000208;

#undef ADDR

uint8_t
IoRegisters::read(uint32_t address) const {
    switch (address) {
        case POSTFLG:
            return system.post_boot_flag;
        case IME:
            return system.interrupt_master_enabler;
        default:
            glogger.warn("Unused IO address read at 0x{:08X}", address);
    }

    return 0xFF;
}

void
IoRegisters::write(uint32_t address, uint8_t byte) {
    switch (address) {
        case POSTFLG:
            system.post_boot_flag = byte & 1;
            break;
        case IME:
            system.interrupt_master_enabler = byte & 1;
            break;
        default:
            glogger.warn("Unused IO address written at 0x{:08X}", address);
    }
    return;
}
}
