#pragma once
#include "lcd.hh"
#include "sound.hh"
#include <cstdint>

namespace matar {
class IoDevices {
  public:
    uint8_t read_byte(uint32_t) const;
    void write_byte(uint32_t, uint8_t);

    uint32_t read_word(uint32_t) const;
    void write_word(uint32_t, uint32_t);

    uint16_t read_halfword(uint32_t) const;
    void write_halfword(uint32_t, uint16_t);

  private:
    struct {
        using u16 = uint16_t;
        bool post_boot_flag;
        bool interrupt_master_enabler;
        u16 interrupt_enable;
        u16 interrupt_request_flags;
        u16 waitstate_control;
        bool low_power_mode;
    } system = {};

    struct lcd lcd     = {};
    struct sound sound = {};
};
}
