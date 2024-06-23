#pragma once

#include "../../../src/util/log.hh"
#include "registers.hh"
#include <cstdint>
#include <cstdio>

namespace matar {

class Bus;

class System {
  public:
    enum class Irq {
        LCD_VBLANK         = 0,
        LCD_HBLANK         = 1,
        LCD_VCOUNTER_MATCH = 2,
        TIMER0_OVERFLOW    = 3,
        TIMER1_OVERFLOW    = 4,
        TIMER2_OVERFLOW    = 5,
        TIMER3_OVERFLOW    = 6,
        SERIAL             = 7,
        DMA0               = 8,
        DMA1               = 9,
        DMA2               = 10,
        DMA3               = 11,
        KEYPAD             = 12,
        GAME_PAK           = 13
    };

    System(Bus& bus)
      : bus(bus) {}

    uint16_t read_halfword(uint32_t address) const;
    void write_halfword(uint32_t address, uint16_t halfword);

    void raise_irq(Irq event) {
        interrupt_request_flags |= 1 << static_cast<uint8_t>(event);
    }

    bool any_irq_is_pending() {
        return interrupt_master_enabler &
               !!(interrupt_enable & interrupt_request_flags);
    }

  private:
    uint16_t interrupt_enable;
    uint16_t interrupt_request_flags;
    bool interrupt_master_enabler;
    bool post_boot_flag;
    WaitstateControl waitstate_control;
    bool low_power_mode;

    Bus& bus;
};
}
