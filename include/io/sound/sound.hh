#pragma once

#include "io/dma/dma.hh"
#include "io/sound/buffer.hh"
#include "io/sound/resampler.hh"
#include "scheduler.hh"
#include <cstdint>
#include <io/sound/registers.hh>

namespace matar {
namespace sound {

static constexpr auto PWM_FREQUENCY = 16780000;

class SoundFIFO {
  public:
    size_t size() { return fifo.size(); }

    void write(uint16_t h) {
        if (fifo.size() + 2 >= MAX_BYTES)
            return;

        fifo.push((int8_t)(h & 0xFF));
        fifo.push((int8_t)((h >> 8) & 0xFF));
    }

    int8_t read() {
        if (fifo.empty()) {
            return 0;
        }

        int8_t v = fifo.front();
        fifo.pop();
        return v;
    }

  private:
    static constexpr size_t MAX_BYTES = 32;
    std::queue<int8_t> fifo;
};

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
class Sound {
    using u16 = uint16_t;
    using u32 = uint32_t;

  public:
    Sound(Dma& dma, Scheduler& scheduler, uint32_t freq_out)
      : dma(dma)
      , scheduler(scheduler)
      , sampling_rate(DEFAULT_SAMPLING_RATE)
      , resampler(sampling_rate, freq_out)
      , buffer(1024) {
        scheduler.schedule_from_now(Task::Type::SAMPLE_PWM,
                                    PWM_FREQUENCY / sampling_rate);
    }

    u16 read_halfword(u32 address) const;
    void write_halfword(u32 address, u16 value);

    void dma_playback(uint8_t timer_id, uint64_t at);
    void sample(uint64_t at);

  private:
    // channel 1
    Ch1Sweep ch1_sweep;
    Ch1Envelope ch1_envelope;
    Ch1FrequencyControl ch1_freq_ctrl;

    // 75726
    // channel 2
    Ch2Envelope ch2_envelope;
    Ch2FrequencyControl ch2_freq_ctrl;

    // channel 3
    Ch3WaveSelect ch3_wave_select;
    Ch3LengthVolume ch3_len_vol;
    Ch3FrequencyControl ch3_freq_ctrl;
    uint16_t ch3_wave_pattern[8];

    // channel 4
    Ch4Envelope ch4_envelope;
    Ch4FrequencyControl ch4_freq_ctrl;

    // control
    LRVolumeControl vol_ctrl;
    DmaControl dma_ctrl;
    SoundOnOff sound_on_off;
    SoundBias sound_bias;

    // fifo
    SoundFIFO fifo_a;
    SoundFIFO fifo_b;

    int8_t dma_value_a;
    int8_t dma_value_b;

    Dma& dma;
    Scheduler& scheduler;
    uint32_t sampling_rate;
    Resampler resampler;
    AudioBuffer buffer;
};
// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
}
}
