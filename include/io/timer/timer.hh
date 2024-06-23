#pragma once

#include <io/sound/sound.hh>
#include "io/system/system.hh"
#include "registers.hh"
#include "scheduler.hh"
#include <cstdint>

namespace matar {
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
static constexpr int NUM_TIMERS = 4;

class Timer {
    using u16 = uint16_t;

  public:
    Timer(Scheduler& scheduler, System& irq, sound::Sound& sound)
      : scheduler(scheduler)
      , irq(irq)
      , sound(sound) {}

    uint16_t read_halfword(uint32_t address) const;
    void write_halfword(uint32_t address, uint16_t halfword);

    void trigger_overflow(uint8_t id, uint64_t at);

  private:
    struct {
        u16 counter;
        u16 reload;
        TimerControl control;
    } timers[NUM_TIMERS];

    void write_and_eval_ctrl(uint8_t id, u16 raw);
    void schedule_overflow(uint8_t id, uint64_t at);

    Scheduler& scheduler;
    System& irq;
    sound::Sound& sound;
};

// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
}
