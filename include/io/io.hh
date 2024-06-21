#pragma once

#include "display/display.hh"
#include "dma.hh"
#include "sound.hh"
#include <cstdint>
#include <memory>

namespace matar {
class Bus; // forward declaration

class IoDevices {
  public:
    IoDevices(std::weak_ptr<Bus>);

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

    display::Display display = {};
    Sound sound              = {};
    Dma dma                  = {};

    std::weak_ptr<Bus> bus;
    friend class Bus;
};
}
