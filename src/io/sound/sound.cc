#include "io/sound/sound.hh"
#include "io/sound/buffer.hh"
#include "util/log.hh"
#include <algorithm>

namespace matar {
namespace sound {

void
Sound::dma_playback(uint8_t timer_id, uint64_t at) {
    if (!sound_on_off.value.psg_fifo_master_ena) {
        return;
    }

    if (dma_ctrl.value.dma_a_timer_sel == timer_id) {
        dma_value_a = fifo_a.read();

        if (fifo_a.size() <= 16) {
            dma.schedule_sound_xfer(FIFO_A, at);
        }
    }

    if (dma_ctrl.value.dma_b_timer_sel == timer_id) {
        dma_value_b = fifo_b.read();
        if (fifo_b.size() <= 16) {
            dma.schedule_sound_xfer(FIFO_B, at);
        }
    }
}

void
Sound::sample(uint64_t at) {
    Sample<float> sample;
    int bias = sound_bias.value.level << 1;

    auto apply_bias = [bias](int value) {
        return std::clamp(value + bias, 0, OUT_MAX) - bias;
    };

    /* Left */
    {
        int left = 0;
        if (dma_ctrl.value.dma_a_en_left) {
            /* multiply by 2 or 4 to get volume gain for 50% and 100%
             * respectively
             */
            left += dma_value_a * (2 << dma_ctrl.value.dma_a_volume);
        }
        if (dma_ctrl.value.dma_b_en_left) {
            left += dma_value_b * (2 << dma_ctrl.value.dma_b_volume);
        }
        sample.left = static_cast<float>(apply_bias(left));
    }

    /* Right */
    {
        int right = 0;
        if (dma_ctrl.value.dma_a_en_right) {
            right += dma_value_a * (2 << dma_ctrl.value.dma_a_volume);
        }
        if (dma_ctrl.value.dma_b_en_right) {
            right += dma_value_b * (2 << dma_ctrl.value.dma_b_volume);
        }
        sample.right = static_cast<float>(apply_bias(right));
    }


    resampler.resample(sample, buffer);

    scheduler.schedule_at(Task::Type::SAMPLE_PWM,
                          at + (PWM_FREQUENCY / sampling_rate));
}
}
}
