#pragma once

#include "display/display.hh"
#include "dma/dma.hh"
#include "sound/sound.hh"
#include "system/system.hh"
#include "timer/timer.hh"
#include <cstdint>

namespace matar {
class IoDevices {
  public:
    IoDevices(Bus& bus, Scheduler& scheduler)
      : display(scheduler, system, dma)
      , sound(dma, scheduler, 44100)
      , dma(bus, scheduler, system)
      , timer(scheduler, system, sound)
      , system(bus) {}

    uint8_t read_byte(uint32_t) const;
    void write_byte(uint32_t, uint8_t);

    uint32_t read_word(uint32_t) const;
    void write_word(uint32_t, uint32_t);

    uint16_t read_halfword(uint32_t) const;
    void write_halfword(uint32_t, uint16_t);

    auto& pram() { return display.get_pram(); }
    const auto& pram() const { return display.get_pram(); }

    auto& vram() { return display.get_vram(); }
    const auto& vram() const { return display.get_vram(); }

    auto& oam() { return display.get_oam(); }
    const auto& oam() const { return display.get_oam(); }

    size_t obj_offset() {return display.obj_offset();}

    void scheduler_event(Task::Type type, uint64_t at);

    bool any_is_interrupt_pending() { return system.any_irq_is_pending(); }

  private:
    display::Display display;
    sound::Sound sound;
    Dma dma;
    Timer timer;
    System system;
};
}
