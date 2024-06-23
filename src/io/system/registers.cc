#include "bus.hh"
#include "util/log.hh"

namespace matar {
static constexpr uint32_t IE      = 0x4000200;
static constexpr uint32_t IF      = 0x4000202;
static constexpr uint32_t WAITCNT = 0x4000204;
static constexpr uint32_t IME     = 0x4000208;
static constexpr uint32_t POSTFLG = 0x4000300;
static constexpr uint32_t HALTCNT = 0x4000301;

uint16_t
System::read_halfword(uint32_t address) const {

    switch (address) {
        case IE: {
            return interrupt_enable;
        }
        case IF: {
            return interrupt_request_flags;
        }
        case WAITCNT: {
            return waitstate_control.read();
        }
        case IME: {
            return interrupt_master_enabler;
        }
        case POSTFLG: {
            return post_boot_flag;
        }
        case HALTCNT: {
            glogger.warn("HALTCNT read");
            return low_power_mode;
        }
        default: {
            glogger.warn("Invalid system I/O address read at 0x{:08X}",
                         address);
        }
    }

    return 0xFFFF;
}

void
System::write_halfword(uint32_t address, uint16_t halfword) {
    switch (address) {
        case IE: {
            interrupt_enable = halfword;
            break;
        }
        case IF: {
            interrupt_request_flags &= ~halfword; /* clear flags */
            break;
        }
        case WAITCNT: {
            waitstate_control.write(halfword);
            bus.update_cycle_map(waitstate_control);
            break;
        }
        case IME: {
            interrupt_master_enabler = !!halfword;
            break;
        }
        case POSTFLG: {
            post_boot_flag = !!halfword;
            break;
        }
        case HALTCNT: {
            glogger.warn("HALTCNT write");
            low_power_mode = !!halfword;
            break;
        }
        default: {
            glogger.warn("Unused system I/O address written at 0x{:08X}",
                         address);
        }
    }
}
}
