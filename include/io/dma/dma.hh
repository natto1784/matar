#pragma once

#include "io/system/system.hh"
#include "registers.hh"
#include "scheduler.hh"
#include <cstdint>

namespace matar {
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
static constexpr int NUM_DMA_CHANS = 4;

class Bus;

class Dma {
    using u64 = uint64_t;
    using u32 = uint32_t;
    using u16 = uint16_t;

  public:
    Dma(Bus& bus, Scheduler& scheduler, System& system)
      : bus(bus)
      , scheduler(scheduler)
      , system(system) {}

    uint16_t read_halfword(uint32_t address) const;
    void write_halfword(uint32_t address, uint16_t halfword);

    void start_transfer(uint8_t id);

    void notify(DmaControl::Timing timing, uint64_t at);
    void schedule_sound_xfer(uint32_t fifo_addr, uint64_t at);

  private:
    void write_and_eval_ctrl(uint8_t id, uint16_t raw);

    struct {
        u64 timestamp;

        /* registers */
        u32 source;
        u32 destination;
        u16 word_count;
        DmaControl control;
    } channels[NUM_DMA_CHANS];

    Bus& bus;
    Scheduler& scheduler;
    System& system;
};
// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
}
