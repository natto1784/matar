#include <cstdint>

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)

/*
  4000060h  2  R/W  SOUND1CNT_L Channel 1 Sweep register       (NR10)
  4000062h  2  R/W  SOUND1CNT_H Channel 1 Duty/Length/Envelope (NR11, NR12)
  4000064h  2  R/W  SOUND1CNT_X Channel 1 Frequency/Control    (NR13, NR14)
  4000066h     -    -           Not used
  4000068h  2  R/W  SOUND2CNT_L Channel 2 Duty/Length/Envelope (NR21, NR22)
  400006Ah     -    -           Not used
  400006Ch  2  R/W  SOUND2CNT_H Channel 2 Frequency/Control    (NR23, NR24)
  400006Eh     -    -           Not used
  4000070h  2  R/W  SOUND3CNT_L Channel 3 Stop/Wave RAM select (NR30)
  4000072h  2  R/W  SOUND3CNT_H Channel 3 Length/Volume        (NR31, NR32)
  4000074h  2  R/W  SOUND3CNT_X Channel 3 Frequency/Control    (NR33, NR34)
  4000076h     -    -           Not used
  4000078h  2  R/W  SOUND4CNT_L Channel 4 Length/Envelope      (NR41, NR42)
  400007Ah     -    -           Not used
  400007Ch  2  R/W  SOUND4CNT_H Channel 4 Frequency/Control    (NR43, NR44)
  400007Eh     -    -           Not used
  4000080h  2  R/W  SOUNDCNT_L  Control Stereo/Volume/Enable   (NR50, NR51)
  4000082h  2  R/W  SOUNDCNT_H  Control Mixing/DMA Control
  4000084h  2  R/W  SOUNDCNT_X  Control Sound on/off           (NR52)
  4000086h     -    -           Not used
  4000088h  2  BIOS SOUNDBIAS   Sound PWM Control
  400008Ah  ..   -    -         Not used
  4000090h 2x10h R/W  WAVE_RAM  Channel 3 Wave Pattern RAM (2 banks!!)
  40000A0h  4    W    FIFO_A    Channel A FIFO, Data 0-3
  40000A4h  4    W    FIFO_B    Channel B FIFO, Data 0-3
*/

struct sound {
    using u16 = uint16_t;

    // channel 1
    u16 ch1_sweep;
    u16 ch1_duty_length_env;
    u16 ch1_freq_control;

    // channel 2
    u16 ch2_duty_length_env;
    u16 ch2_freq_control;

    // channel 3
    u16 ch3_stop_wave_ram_select;
    u16 ch3_length_volume;
    u16 ch3_freq_control;
    u16 ch3_wave_pattern[8];

    // channel 4
    u16 ch4_length_env;
    u16 ch4_freq_control;

    // control
    u16 ctrl_stereo_volume;
    u16 ctrl_mixing;
    u16 ctrl_sound_on_off;
    u16 pwm_control;

    // fifo
    u16 fifo_a[2];
    u16 fifo_b[2];
};

// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
