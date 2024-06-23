#pragma once

#include "io/sound/buffer.hh"
#include "io/sound/registers.hh"

#include <cmath>
#include <numbers>

namespace matar {
namespace sound {

class Resampler {
  public:
    Resampler(uint32_t freq_in, uint32_t freq_out)
      : freq_in(static_cast<float>(freq_in))
      , freq_out(static_cast<float>(freq_out)) {}

    void resample(Sample<float> s1, AudioBuffer& buffer) {
        Sample<float> s1_;

        while (phase < 1) {
            float weight =
              (1.0f - std::cosf(std::numbers::pi_v<float> * phase)) / 2;

            s1_.left  = s0.left * weight + s1.left * (1.0f - weight);
            s1_.right = s0.right * weight + s1.right * (1.0f - weight);

            buffer.push({ .left  = static_cast<int16_t>(std::roundl(s1_.left) *
                                                       OUT_TO_I16_MUL),
                          .right = static_cast<int16_t>(std::roundl(s1_.right) *
                                                        OUT_TO_I16_MUL) });

            phase += freq_in / freq_out;
        }

        phase -= 1;
        s0 = s1;
    }

    void set_freq_in(uint32_t freq_in) {
        this->freq_in = static_cast<float>(freq_in);
    }
    uint32_t get_freq_in() const { return static_cast<uint32_t>(freq_in); }

  private:
    float freq_in;
    float freq_out;
    float phase = 0;
    Sample<float> s0{ 0, 0 };
};
}
}
