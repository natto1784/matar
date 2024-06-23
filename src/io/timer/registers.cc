#include "io/timer/timer.hh"
#include "util/log.hh"

namespace matar {

static constexpr uint32_t TM0CNT_L = 0x4000100;
static constexpr uint32_t TM0CNT_H = 0x4000102;
static constexpr uint32_t TM1CNT_L = 0x4000104;
static constexpr uint32_t TM1CNT_H = 0x4000106;
static constexpr uint32_t TM2CNT_L = 0x4000108;
static constexpr uint32_t TM2CNT_H = 0x400010A;
static constexpr uint32_t TM3CNT_L = 0x400010C;
static constexpr uint32_t TM3CNT_H = 0x400010E;

uint16_t
Timer::read_halfword(uint32_t address) const {

    switch (address) {
        case TM0CNT_L: {
            return timers[0].counter;
        }
        case TM0CNT_H: {
            return timers[0].control.read();
        }
        case TM1CNT_L: {
            return timers[1].counter;
        }
        case TM1CNT_H: {
            return timers[1].control.read();
        }
        case TM2CNT_L: {
            return timers[2].counter;
        }
        case TM2CNT_H: {
            return timers[2].control.read();
        }
        case TM3CNT_L: {
            return timers[3].counter;
        }
        case TM3CNT_H: {
            return timers[3].control.read();
        }
        default: {
            glogger.warn("Invalid timer I/O address read at 0x{:08X}", address);
        }
    }

    return 0xFFFF;
}

void
Timer::write_halfword(uint32_t address, uint16_t halfword) {
    switch (address) {
        case TM0CNT_L: {
            timers[0].reload = halfword;
            break;
        }
        case TM0CNT_H: {
            write_and_eval_ctrl(0, halfword);
            break;
        }
        case TM1CNT_L: {
            timers[1].reload = halfword;
            break;
        }
        case TM1CNT_H: {
            write_and_eval_ctrl(1, halfword);
            break;
        }
        case TM2CNT_L: {
            timers[2].reload = halfword;
            break;
        }
        case TM2CNT_H: {
            write_and_eval_ctrl(2, halfword);
            break;
        }
        case TM3CNT_L: {
            timers[3].reload = halfword;
            break;
        }
        case TM3CNT_H: {
            write_and_eval_ctrl(3, halfword);
            break;
        }
        default: {
            glogger.warn("Unused timer I/O address written at 0x{:08X}",
                         address);
        }
    }
}
}
