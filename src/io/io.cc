#include "io/io.hh"
#include "util/bits.hh"
#include "util/log.hh"

namespace matar {
#define ADDR static constexpr uint32_t

// lcd
ADDR DISPCNT  = 0x4000000;
ADDR DISPSTAT = 0x4000004;
ADDR VCOUNT   = 0x4000006;
ADDR BG0CNT   = 0x4000008;
ADDR BG1CNT   = 0x400000A;
ADDR BG2CNT   = 0x400000C;
ADDR BG3CNT   = 0x400000E;
ADDR BG0HOFS  = 0x4000010;
ADDR BG0VOFS  = 0x4000012;
ADDR BG1HOFS  = 0x4000014;
ADDR BG1VOFS  = 0x4000016;
ADDR BG2HOFS  = 0x4000018;
ADDR BG2VOFS  = 0x400001A;
ADDR BG3HOFS  = 0x400001C;
ADDR BG3VOFS  = 0x400001E;
ADDR BG2PA    = 0x4000020;
ADDR BG2PB    = 0x4000022;
ADDR BG2PC    = 0x4000024;
ADDR BG2PD    = 0x4000026;
ADDR BG2X_L   = 0x4000028;
ADDR BG2X_H   = 0x400002A;
ADDR BG2Y_L   = 0x400002C;
ADDR BG2Y_H   = 0x400002E;
ADDR BG3PA    = 0x4000030;
ADDR BG3PB    = 0x4000032;
ADDR BG3PC    = 0x4000034;
ADDR BG3PD    = 0x4000036;
ADDR BG3X_L   = 0x4000038;
ADDR BG3X_H   = 0x400003A;
ADDR BG3Y_L   = 0x400003C;
ADDR BG3Y_H   = 0x400003E;
ADDR WIN0H    = 0x4000040;
ADDR WIN1H    = 0x4000042;
ADDR WIN0V    = 0x4000044;
ADDR WIN1V    = 0x4000046;
ADDR WININ    = 0x4000048;
ADDR WINOUT   = 0x400004A;
ADDR MOSAIC   = 0x400004C;
ADDR BLDCNT   = 0x4000050;
ADDR BLDALPHA = 0x4000052;
ADDR BLDY     = 0x4000054;

// sound
ADDR SOUND1CNT_L = 0x4000060;
ADDR SOUND1CNT_H = 0x4000062;
ADDR SOUND1CNT_X = 0x4000064;
ADDR SOUND2CNT_L = 0x4000068;
ADDR SOUND2CNT_H = 0x400006C;
ADDR SOUND3CNT_L = 0x4000070;
ADDR SOUND3CNT_H = 0x4000072;
ADDR SOUND3CNT_X = 0x4000074;
ADDR SOUND4CNT_L = 0x4000078;
ADDR SOUND4CNT_H = 0x400007C;
ADDR SOUNDCNT_L  = 0x4000080;
ADDR SOUNDCNT_H  = 0x4000082;
ADDR SOUNDCNT_X  = 0x4000084;
ADDR SOUNDBIAS   = 0x4000088;
ADDR WAVE_RAM0_L = 0x4000090;
ADDR WAVE_RAM0_H = 0x4000092;
ADDR WAVE_RAM1_L = 0x4000094;
ADDR WAVE_RAM1_H = 0x4000096;
ADDR WAVE_RAM2_L = 0x4000098;
ADDR WAVE_RAM2_H = 0x400009A;
ADDR WAVE_RAM3_L = 0x400009C;
ADDR WAVE_RAM3_H = 0x400009E;
ADDR FIFO_A_L    = 0x40000A0;
ADDR FIFO_A_H    = 0x40000A2;
ADDR FIFO_B_L    = 0x40000A4;
ADDR FIFO_B_H    = 0x40000A6;

// dma
ADDR DMA0SAD   = 0x40000B0;
ADDR DMA0DAD   = 0x40000B4;
ADDR DMA0CNT_L = 0x40000B8;
ADDR DMA0CNT_H = 0x40000BA;
ADDR DMA1SAD   = 0x40000BC;
ADDR DMA1DAD   = 0x40000C0;
ADDR DMA1CNT_L = 0x40000C4;
ADDR DMA1CNT_H = 0x40000C6;
ADDR DMA2SAD   = 0x40000C8;
ADDR DMA2DAD   = 0x40000CC;
ADDR DMA2CNT_L = 0x40000D0;
ADDR DMA2CNT_H = 0x40000D2;
ADDR DMA3SAD   = 0x40000D4;
ADDR DMA3DAD   = 0x40000D8;
ADDR DMA3CNT_L = 0x40000DC;
ADDR DMA3CNT_H = 0x40000DE;

// system
ADDR POSTFLG = 0x4000300;
ADDR IME     = 0x4000208;
ADDR IE      = 0x4000200;
ADDR IF      = 0x4000202;
ADDR WAITCNT = 0x4000204;
ADDR HALTCNT = 0x4000301;

#undef ADDR

IoDevices::IoDevices(std::weak_ptr<Bus> bus)
  : bus(bus) {}

uint8_t
IoDevices::read_byte(uint32_t address) const {
    uint16_t halfword = read_halfword(address & ~1);

    if (address & 1)
        halfword >>= 8;

    return halfword & 0xFF;
}

void
IoDevices::write_byte(uint32_t address, uint8_t byte) {
    uint16_t halfword = read_halfword(address & ~1);

    if (address & 1)
        write_halfword(address & ~1,
                       (static_cast<uint16_t>(byte) << 8) | (halfword & 0xFF));
    else
        write_halfword(address & ~1,
                       (static_cast<uint16_t>(byte) | (halfword & 0xFF00)));
}

uint32_t
IoDevices::read_word(uint32_t address) const {
    return read_halfword(address) | read_halfword(address + 2) << 16;
}

void
IoDevices::write_word(uint32_t address, uint32_t word) {
    write_halfword(address, word & 0xFFFF);
    write_halfword(address + 2, (word >> 16) & 0xFFFF);
}

uint16_t
IoDevices::read_halfword(uint32_t address) const {
    switch (address) {

#define READ(name, var)                                                        \
    case name:                                                                 \
        return var;

            // lcd
        case DISPCNT:
            return display.lcd_control.read();
        case DISPSTAT:
            return display.general_lcd_status.read();
        case BG0CNT:
            return display.bg_control[0].read();
        case BG1CNT:
            return display.bg_control[1].read();
        case BG2CNT:
            return display.bg_control[2].read();
        case BG3CNT:
            return display.bg_control[3].read();

            READ(VCOUNT, display.vertical_counter)
            READ(WININ, display.inside_win_0_1)
            READ(WINOUT, display.outside_win)
            READ(BLDCNT, display.color_special_effects_selection)
            READ(BLDALPHA, display.alpha_blending_coefficients)

            // sound
            READ(SOUND1CNT_L, sound.ch1_sweep)
            READ(SOUND1CNT_H, sound.ch1_duty_length_env)
            READ(SOUND1CNT_X, sound.ch1_freq_control)
            READ(SOUND2CNT_L, sound.ch2_duty_length_env)
            READ(SOUND2CNT_H, sound.ch2_freq_control)
            READ(SOUND3CNT_L, sound.ch3_stop_wave_ram_select)
            READ(SOUND3CNT_H, sound.ch3_length_volume)
            READ(SOUND3CNT_X, sound.ch3_freq_control)
            READ(WAVE_RAM0_L, sound.ch3_wave_pattern[0]);
            READ(WAVE_RAM0_H, sound.ch3_wave_pattern[1]);
            READ(WAVE_RAM1_L, sound.ch3_wave_pattern[2]);
            READ(WAVE_RAM1_H, sound.ch3_wave_pattern[3]);
            READ(WAVE_RAM2_L, sound.ch3_wave_pattern[4]);
            READ(WAVE_RAM2_H, sound.ch3_wave_pattern[5]);
            READ(WAVE_RAM3_L, sound.ch3_wave_pattern[6]);
            READ(WAVE_RAM3_H, sound.ch3_wave_pattern[7]);
            READ(SOUND4CNT_L, sound.ch4_length_env);
            READ(SOUND4CNT_H, sound.ch4_freq_control);
            READ(SOUNDCNT_L, sound.ctrl_stereo_volume);
            READ(SOUNDCNT_H, sound.ctrl_mixing);
            READ(SOUNDCNT_X, sound.ctrl_sound_on_off);
            READ(SOUNDBIAS, sound.pwm_control);

            // dma
        case DMA0CNT_H:
            return dma.channels[0].control.read();
        case DMA1CNT_H:
            return dma.channels[1].control.read();
        case DMA2CNT_H:
            return dma.channels[2].control.read();
        case DMA3CNT_H:
            return dma.channels[3].control.read();

            READ(DMA0SAD, dma.channels[0].source[0]);
            READ(DMA0SAD + 2, dma.channels[0].source[1]);
            READ(DMA0DAD, dma.channels[0].destination[0]);
            READ(DMA0DAD + 2, dma.channels[0].destination[1]);
            READ(DMA0CNT_L, dma.channels[0].word_count);
            READ(DMA1SAD, dma.channels[1].source[0]);
            READ(DMA1SAD + 2, dma.channels[1].source[1]);
            READ(DMA1DAD, dma.channels[1].destination[0]);
            READ(DMA1DAD + 2, dma.channels[1].destination[1]);
            READ(DMA1CNT_L, dma.channels[1].word_count);
            READ(DMA2SAD, dma.channels[2].source[0]);
            READ(DMA2SAD + 2, dma.channels[2].source[1]);
            READ(DMA2DAD, dma.channels[2].destination[0]);
            READ(DMA2DAD + 2, dma.channels[2].destination[1]);
            READ(DMA2CNT_L, dma.channels[2].word_count);
            READ(DMA3SAD, dma.channels[3].source[0]);
            READ(DMA3SAD + 2, dma.channels[3].source[1]);
            READ(DMA3DAD, dma.channels[3].destination[0]);
            READ(DMA3DAD + 2, dma.channels[3].destination[1]);
            READ(DMA3CNT_L, dma.channels[3].word_count);

            // system
            READ(POSTFLG, system.post_boot_flag)
            READ(IME, system.interrupt_master_enabler)
            READ(IE, system.interrupt_enable);
            READ(IF, system.interrupt_request_flags);
            READ(WAITCNT, system.waitstate_control);

#undef READ

        default:
            glogger.warn("Unused IO address read at 0x{:08X}", address);
    }

    return 0xFF;
}

void
IoDevices::write_halfword(uint32_t address, uint16_t halfword) {
    // set lower 16 bits for reference points (BG 2/3)
    auto ref_low = [](uint32_t original, uint16_t low) {
        return static_cast<int32_t>((original & 0xFFFF0000) | low);
    };

    // set upper 12 bits for reference points (BG 2/3)
    // and sign extend
    auto ref_high = [](uint32_t original, uint16_t high) {
        return static_cast<int32_t>(
          ((((high & 0xFFF) << 16) | (original & 0xFFFF)) << 4) >> 4);
    };

    switch (address) {

#define WRITE(name, var)                                                       \
    case name:                                                                 \
        var = halfword;                                                        \
        break;

#define WRITE_2(name, var, val)                                                \
    case name:                                                                 \
        var = val;                                                             \
        break;

        // lcd
        case DISPCNT:
            display.lcd_control.write(halfword);
            break;
        case DISPSTAT:
            display.general_lcd_status.write(halfword);
            break;
        case BG0CNT:
            display.bg_control[0].write(halfword);
            break;
        case BG1CNT:
            display.bg_control[1].write(halfword);
            break;
        case BG2CNT:
            display.bg_control[2].write(halfword);
            break;
        case BG3CNT:
            display.bg_control[3].write(halfword);
            break;

            WRITE(BG0HOFS, display.bg0_offset.x)
            WRITE(BG0VOFS, display.bg0_offset.y)
            WRITE(BG1HOFS, display.bg1_offset.x)
            WRITE(BG1VOFS, display.bg1_offset.y)
            WRITE(BG2HOFS, display.bg2_offset.x)
            WRITE(BG2VOFS, display.bg2_offset.y)
            WRITE(BG3HOFS, display.bg3_offset.x)
            WRITE(BG3VOFS, display.bg3_offset.y)
            WRITE(BG2PA, display.bg2_rot_scale.a)
            WRITE(BG2PB, display.bg2_rot_scale.b)
            WRITE(BG2PC, display.bg2_rot_scale.c)
            WRITE(BG2PD, display.bg2_rot_scale.d)
            WRITE_2(BG2X_L,
                    display.bg2_rot_scale.ref.x,
                    ref_low(display.bg2_rot_scale.ref.x, halfword));
            WRITE_2(BG2X_H,
                    display.bg2_rot_scale.ref.x,
                    ref_high(display.bg2_rot_scale.ref.x, halfword));
            WRITE_2(BG2Y_L,
                    display.bg2_rot_scale.ref.y,
                    ref_low(display.bg2_rot_scale.ref.y, halfword));
            WRITE_2(BG2Y_H,
                    display.bg2_rot_scale.ref.y,
                    ref_high(display.bg2_rot_scale.ref.y, halfword));
            WRITE(BG3PA, display.bg3_rot_scale.a)
            WRITE(BG3PB, display.bg3_rot_scale.b)
            WRITE(BG3PC, display.bg3_rot_scale.c)
            WRITE(BG3PD, display.bg3_rot_scale.d)
            WRITE_2(BG3X_L,
                    display.bg3_rot_scale.ref.x,
                    ref_low(display.bg3_rot_scale.ref.x, halfword));
            WRITE_2(BG3X_H,
                    display.bg3_rot_scale.ref.x,
                    ref_high(display.bg3_rot_scale.ref.x, halfword));
            WRITE_2(BG3Y_L,
                    display.bg3_rot_scale.ref.y,
                    ref_low(display.bg3_rot_scale.ref.y, halfword));
            WRITE_2(BG3Y_H,
                    display.bg3_rot_scale.ref.y,
                    ref_high(display.bg3_rot_scale.ref.y, halfword));
            WRITE(WIN0H, display.win0_horizontal_dimensions)
            WRITE(WIN1H, display.win1_horizontal_dimensions)
            WRITE(WIN0V, display.win0_vertical_dimensions)
            WRITE(WIN1V, display.win1_vertical_dimensions)
            WRITE(WININ, display.inside_win_0_1)
            WRITE(WINOUT, display.outside_win)
            WRITE(MOSAIC, display.mosaic_size)
            WRITE(BLDCNT, display.color_special_effects_selection)
            WRITE(BLDALPHA, display.alpha_blending_coefficients)
            WRITE(BLDY, display.brightness_coefficient)

            // sound
            WRITE(SOUND1CNT_L, sound.ch1_sweep)
            WRITE(SOUND1CNT_H, sound.ch1_duty_length_env)
            WRITE(SOUND1CNT_X, sound.ch1_freq_control)
            WRITE(SOUND2CNT_L, sound.ch2_duty_length_env)
            WRITE(SOUND2CNT_H, sound.ch2_freq_control)
            WRITE(SOUND3CNT_L, sound.ch3_stop_wave_ram_select)
            WRITE(SOUND3CNT_H, sound.ch3_length_volume)
            WRITE(SOUND3CNT_X, sound.ch3_freq_control)
            WRITE(WAVE_RAM0_L, sound.ch3_wave_pattern[0]);
            WRITE(WAVE_RAM0_H, sound.ch3_wave_pattern[1]);
            WRITE(WAVE_RAM1_L, sound.ch3_wave_pattern[2]);
            WRITE(WAVE_RAM1_H, sound.ch3_wave_pattern[3]);
            WRITE(WAVE_RAM2_L, sound.ch3_wave_pattern[4]);
            WRITE(WAVE_RAM2_H, sound.ch3_wave_pattern[5]);
            WRITE(WAVE_RAM3_L, sound.ch3_wave_pattern[6]);
            WRITE(WAVE_RAM3_H, sound.ch3_wave_pattern[7]);
            WRITE(SOUND4CNT_L, sound.ch4_length_env);
            WRITE(SOUND4CNT_H, sound.ch4_freq_control);
            WRITE(SOUNDCNT_L, sound.ctrl_stereo_volume);
            WRITE(SOUNDCNT_H, sound.ctrl_mixing);
            WRITE(SOUNDCNT_X, sound.ctrl_sound_on_off);
            WRITE(SOUNDBIAS, sound.pwm_control);
            WRITE(FIFO_A_L, sound.fifo_a[0]);
            WRITE(FIFO_A_H, sound.fifo_a[1]);
            WRITE(FIFO_B_L, sound.fifo_b[0]);
            WRITE(FIFO_B_H, sound.fifo_b[1]);

            // dma
        case DMA0CNT_H:
            dma.channels[0].control.write(halfword);
            break;
        case DMA1CNT_H:
            dma.channels[1].control.write(halfword);
            break;
        case DMA2CNT_H:
            dma.channels[2].control.write(halfword);
            break;
        case DMA3CNT_H:
            dma.channels[3].control.write(halfword);
            break;

            WRITE(DMA0SAD, dma.channels[0].source[0]);
            WRITE(DMA0SAD + 2, dma.channels[0].source[1]);
            WRITE(DMA0DAD, dma.channels[0].destination[0]);
            WRITE(DMA0DAD + 2, dma.channels[0].destination[1]);
            WRITE(DMA0CNT_L, dma.channels[0].word_count);
            WRITE(DMA1SAD, dma.channels[1].source[0]);
            WRITE(DMA1SAD + 2, dma.channels[1].source[1]);
            WRITE(DMA1DAD, dma.channels[1].destination[0]);
            WRITE(DMA1DAD + 2, dma.channels[1].destination[1]);
            WRITE(DMA1CNT_L, dma.channels[1].word_count);
            WRITE(DMA2SAD, dma.channels[2].source[0]);
            WRITE(DMA2SAD + 2, dma.channels[2].source[1]);
            WRITE(DMA2DAD, dma.channels[2].destination[0]);
            WRITE(DMA2DAD + 2, dma.channels[2].destination[1]);
            WRITE(DMA2CNT_L, dma.channels[2].word_count);
            WRITE(DMA3SAD, dma.channels[3].source[0]);
            WRITE(DMA3SAD + 2, dma.channels[3].source[1]);
            WRITE(DMA3DAD, dma.channels[3].destination[0]);
            WRITE(DMA3DAD + 2, dma.channels[3].destination[1]);
            WRITE(DMA3CNT_L, dma.channels[3].word_count);

            // system
            WRITE_2(POSTFLG, system.post_boot_flag, halfword & 1)
            WRITE_2(IME, system.interrupt_master_enabler, halfword & 1)
            WRITE(IE, system.interrupt_enable);
            WRITE(IF, system.interrupt_request_flags);
            WRITE(WAITCNT, system.waitstate_control);
            WRITE_2(HALTCNT, system.low_power_mode, get_bit(halfword, 7));

#undef WRITE
#undef WRITE_2

        default:
            glogger.warn("Unused IO address written at 0x{:08X}", address);
    }
    return;
}

}
