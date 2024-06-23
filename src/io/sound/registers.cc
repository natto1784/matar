#include <io/sound/sound.hh>
#include <util/log.hh>

namespace matar {
namespace sound {

static constexpr uint32_t SOUND1CNT_L = 0x4000060;
static constexpr uint32_t SOUND1CNT_H = 0x4000062;
static constexpr uint32_t SOUND1CNT_X = 0x4000064;
static constexpr uint32_t SOUND2CNT_L = 0x4000068;
static constexpr uint32_t SOUND2CNT_H = 0x400006C;
static constexpr uint32_t SOUND3CNT_L = 0x4000070;
static constexpr uint32_t SOUND3CNT_H = 0x4000072;
static constexpr uint32_t SOUND3CNT_X = 0x4000074;
static constexpr uint32_t SOUND4CNT_L = 0x4000078;
static constexpr uint32_t SOUND4CNT_H = 0x400007C;
static constexpr uint32_t SOUNDCNT_L  = 0x4000080;
static constexpr uint32_t SOUNDCNT_H  = 0x4000082;
static constexpr uint32_t SOUNDCNT_X  = 0x4000084;
static constexpr uint32_t SOUNDBIAS   = 0x4000088;
static constexpr uint32_t WAVE_RAM0_L = 0x4000090;
static constexpr uint32_t WAVE_RAM0_H = 0x4000092;
static constexpr uint32_t WAVE_RAM1_L = 0x4000094;
static constexpr uint32_t WAVE_RAM1_H = 0x4000096;
static constexpr uint32_t WAVE_RAM2_L = 0x4000098;
static constexpr uint32_t WAVE_RAM2_H = 0x400009A;
static constexpr uint32_t WAVE_RAM3_L = 0x400009C;
static constexpr uint32_t WAVE_RAM3_H = 0x400009E;
static constexpr uint32_t FIFO_A_L    = FIFO_A;
static constexpr uint32_t FIFO_A_H    = FIFO_A + 2;
static constexpr uint32_t FIFO_B_L    = FIFO_B;
static constexpr uint32_t FIFO_B_H    = FIFO_B + 2;

uint16_t
Sound::read_halfword(uint32_t address) const {

    switch (address) {
        case SOUND1CNT_L: {
            return ch1_sweep.read();
        }
        case SOUND1CNT_H: {
            return ch1_envelope.read();
        }
        case SOUND1CNT_X: {
            return ch1_freq_ctrl.read();
        }
        case SOUND2CNT_L: {
            return ch2_envelope.read();
        }
        case SOUND2CNT_H: {
            return ch2_freq_ctrl.read();
        }
        case SOUND3CNT_L: {
            return ch3_wave_select.read();
        }
        case SOUND3CNT_H: {
            return ch3_len_vol.read();
        }
        case SOUND3CNT_X: {
            return ch3_freq_ctrl.read();
        }
        case WAVE_RAM0_L: {
            return ch3_wave_pattern[0];
        }
        case WAVE_RAM0_H: {
            return ch3_wave_pattern[1];
        }
        case WAVE_RAM1_L: {
            return ch3_wave_pattern[2];
        }
        case WAVE_RAM1_H: {
            return ch3_wave_pattern[3];
        }
        case WAVE_RAM2_L: {
            return ch3_wave_pattern[4];
        }
        case WAVE_RAM2_H: {
            return ch3_wave_pattern[5];
        }
        case WAVE_RAM3_L: {
            return ch3_wave_pattern[6];
        }
        case WAVE_RAM3_H: {
            return ch3_wave_pattern[7];
        }
        case SOUND4CNT_L: {
            return ch4_envelope.read();
        }
        case SOUND4CNT_H: {
            return ch4_freq_ctrl.read();
        }
        case SOUNDCNT_L: {
            return vol_ctrl.read();
        }
        case SOUNDCNT_H: {
            return dma_ctrl.read();
        }
        case SOUNDCNT_X: {
            return sound_on_off.read();
        }
        case SOUNDBIAS: {
            return sound_bias.read();
        }
        default: {
            glogger.warn("Unused Sound IO address read at 0x{:08X}", address);
        }
    }

    return 0xFFFF;
}

void
Sound::write_halfword(uint32_t address, uint16_t halfword) {
    switch (address) {
        case SOUND1CNT_L: {
            ch1_sweep.write(halfword);
            break;
        }
        case SOUND1CNT_H: {
            ch1_envelope.write(halfword);
            break;
        }
        case SOUND1CNT_X: {
            ch1_freq_ctrl.write(halfword);
            break;
        }
        case SOUND2CNT_L: {
            ch2_envelope.write(halfword);
            break;
        }
        case SOUND2CNT_H: {
            ch2_freq_ctrl.write(halfword);
            break;
        }
        case SOUND3CNT_L: {
            ch3_wave_select.write(halfword);
            break;
        }
        case SOUND3CNT_H: {
            ch3_len_vol.write(halfword);
            break;
        }
        case SOUND3CNT_X: {
            ch3_freq_ctrl.write(halfword);
            break;
        }
        case WAVE_RAM0_L: {
            ch3_wave_pattern[0] = halfword;
            break;
        }
        case WAVE_RAM0_H: {
            ch3_wave_pattern[1] = halfword;
            break;
        }
        case WAVE_RAM1_L: {
            ch3_wave_pattern[2] = halfword;
            break;
        }
        case WAVE_RAM1_H: {
            ch3_wave_pattern[3] = halfword;
            break;
        }
        case WAVE_RAM2_L: {
            ch3_wave_pattern[4] = halfword;
            break;
        }
        case WAVE_RAM2_H: {
            ch3_wave_pattern[5] = halfword;
            break;
        }
        case WAVE_RAM3_L: {
            ch3_wave_pattern[6] = halfword;
            break;
        }
        case WAVE_RAM3_H: {
            ch3_wave_pattern[7] = halfword;
            break;
        }
        case SOUND4CNT_L: {
            ch4_envelope.write(halfword);
            break;
        }
        case SOUND4CNT_H: {
            ch4_freq_ctrl.write(halfword);
            break;
        }
        case SOUNDCNT_L: {
            vol_ctrl.write(halfword);
            break;
        }
        case SOUNDCNT_H: {
            dma_ctrl.write(halfword);
            break;
        }
        case SOUNDCNT_X: {
            sound_on_off.write(halfword);
            break;
        }
        case SOUNDBIAS: {
            sound_bias.write(halfword);
            sampling_rate = DEFAULT_SAMPLING_RATE << sound_bias.value.amp_resolution;
            resampler.set_freq_in(sampling_rate);
            dbg(sampling_rate);
            int level = sound_bias.value.level;
            dbg(level);

            break;
        }
        case FIFO_A_L:
        case FIFO_A_H: {
            fifo_a.write(halfword);
            break;
        }
        case FIFO_B_L:
        case FIFO_B_H: {
            fifo_b.write(halfword);
            break;
        }
        default: {
            glogger.warn("Unused sound IO address written at 0x{:08X}",
                         address);
        }
    }
}
}
}
