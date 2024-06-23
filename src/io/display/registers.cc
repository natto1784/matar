#include "io/display/display.hh"
#include "util/bits.hh"
#include "util/log.hh"

namespace matar {
namespace display {

static constexpr uint32_t DISPCNT  = 0x4000000;
static constexpr uint32_t DISPSTAT = 0x4000004;
static constexpr uint32_t VCOUNT   = 0x4000006;
static constexpr uint32_t BG0CNT   = 0x4000008;
static constexpr uint32_t BG1CNT   = 0x400000A;
static constexpr uint32_t BG2CNT   = 0x400000C;
static constexpr uint32_t BG3CNT   = 0x400000E;
static constexpr uint32_t BG0HOFS  = 0x4000010;
static constexpr uint32_t BG0VOFS  = 0x4000012;
static constexpr uint32_t BG1HOFS  = 0x4000014;
static constexpr uint32_t BG1VOFS  = 0x4000016;
static constexpr uint32_t BG2HOFS  = 0x4000018;
static constexpr uint32_t BG2VOFS  = 0x400001A;
static constexpr uint32_t BG3HOFS  = 0x400001C;
static constexpr uint32_t BG3VOFS  = 0x400001E;
static constexpr uint32_t BG2PA    = 0x4000020;
static constexpr uint32_t BG2PB    = 0x4000022;
static constexpr uint32_t BG2PC    = 0x4000024;
static constexpr uint32_t BG2PD    = 0x4000026;
static constexpr uint32_t BG2X_L   = 0x4000028;
static constexpr uint32_t BG2X_H   = 0x400002A;
static constexpr uint32_t BG2Y_L   = 0x400002C;
static constexpr uint32_t BG2Y_H   = 0x400002E;
static constexpr uint32_t BG3PA    = 0x4000030;
static constexpr uint32_t BG3PB    = 0x4000032;
static constexpr uint32_t BG3PC    = 0x4000034;
static constexpr uint32_t BG3PD    = 0x4000036;
static constexpr uint32_t BG3X_L   = 0x4000038;
static constexpr uint32_t BG3X_H   = 0x400003A;
static constexpr uint32_t BG3Y_L   = 0x400003C;
static constexpr uint32_t BG3Y_H   = 0x400003E;
static constexpr uint32_t WIN0H    = 0x4000040;
static constexpr uint32_t WIN1H    = 0x4000042;
static constexpr uint32_t WIN0V    = 0x4000044;
static constexpr uint32_t WIN1V    = 0x4000046;
static constexpr uint32_t WININ    = 0x4000048;
static constexpr uint32_t WINOUT   = 0x400004A;
static constexpr uint32_t MOSAIC   = 0x400004C;
static constexpr uint32_t BLDCNT   = 0x4000050;
static constexpr uint32_t BLDALPHA = 0x4000052;
static constexpr uint32_t BLDY     = 0x4000054;

uint16_t
Display::read_halfword(uint32_t address) const {

    switch (address) {
        case DISPCNT: {
            return lcd_control.read();
        }
        case DISPSTAT: {
            return lcd_status.read();
        }
        case BG0CNT: {
            return bg_control[0].read();
        }
        case BG1CNT: {
            return bg_control[1].read();
        }
        case BG2CNT: {
            return bg_control[2].read();
        }
        case BG3CNT: {
            return bg_control[3].read();
        }
        case WININ: {
            return win1.read() << 8 | win0.read();
        }
        case WINOUT: {
            return win_obj.read() << 8 | win_out.read();
        }
        case BLDCNT: {
            return blend_control.read();
        }
        case BLDALPHA: {
            return alpha_coeff.read();
        }
        case VCOUNT: {
            return vertical_counter;
        }
        default: {
            glogger.warn("Invalid display IO address read at 0x{:08X}",
                         address);
        }
    }

    return 0xFFFF;
}

void
Display::write_halfword(uint32_t address, uint16_t halfword) {
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
        case DISPCNT: {
            lcd_control.write(halfword);
            break;
        }
        case DISPSTAT: {
            lcd_status.write(halfword);
            break;
        }
        case BG0CNT: {
            bg_control[0].write(halfword);
            break;
        }
        case BG1CNT: {
            bg_control[1].write(halfword);
            break;
        }
        case BG2CNT: {
            bg_control[2].write(halfword);
            break;
        }
        case BG3CNT: {
            bg_control[3].write(halfword);
            break;
        }
        case BG0HOFS: {
            bg_offset[0].x = halfword;
            break;
        }
        case BG0VOFS: {
            bg_offset[0].y = halfword;
            break;
        }
        case BG1HOFS: {
            bg_offset[1].x = halfword;
            break;
        }
        case BG1VOFS: {
            bg_offset[1].y = halfword;
            break;
        }
        case BG2HOFS: {
            bg_offset[2].x = halfword;
            break;
        }
        case BG2VOFS: {
            bg_offset[2].y = halfword;
            break;
        }
        case BG3HOFS: {
            bg_offset[3].x = halfword;
            break;
        }
        case BG3VOFS: {
            bg_offset[3].y = halfword;
            break;
        }
        case BG2PA: {
            bg2_rot_scale.a = static_cast<int16_t>(halfword);
            break;
        }
        case BG2PB: {
            bg2_rot_scale.b = static_cast<int16_t>(halfword);
            break;
        }
        case BG2PC: {
            bg2_rot_scale.c = static_cast<int16_t>(halfword);
            break;
        }
        case BG2PD: {
            bg2_rot_scale.d = static_cast<int16_t>(halfword);
            break;
        }
        case BG2X_L: {
            bg2_rot_scale.ref.x = ref_low(bg2_rot_scale.ref.x, halfword);
            break;
        }
        case BG2X_H: {
            bg2_rot_scale.ref.x = ref_high(bg2_rot_scale.ref.x, halfword);
            break;
        }
        case BG2Y_L: {
            bg2_rot_scale.ref.y = ref_low(bg2_rot_scale.ref.y, halfword);
            break;
        }
        case BG2Y_H: {
            bg2_rot_scale.ref.y = ref_high(bg2_rot_scale.ref.y, halfword);
            break;
        }
        case BG3PA: {
            bg3_rot_scale.a = static_cast<int16_t>(halfword);
            break;
        }
        case BG3PB: {
            bg3_rot_scale.b = static_cast<int16_t>(halfword);
            break;
        }
        case BG3PC: {
            bg3_rot_scale.c = static_cast<int16_t>(halfword);
            break;
        }
        case BG3PD: {
            bg3_rot_scale.d = static_cast<int16_t>(halfword);
            break;
        }
        case BG3X_L: {
            bg3_rot_scale.ref.x = ref_low(bg3_rot_scale.ref.x, halfword);
            break;
        }
        case BG3X_H: {
            bg3_rot_scale.ref.x = ref_high(bg3_rot_scale.ref.x, halfword);
            break;
        }
        case BG3Y_L: {
            bg3_rot_scale.ref.y = ref_low(bg3_rot_scale.ref.y, halfword);
            break;
        }
        case BG3Y_H: {
            bg3_rot_scale.ref.y = ref_high(bg3_rot_scale.ref.y, halfword);
            break;
        }
        case WIN0H: {
            win0_bot_right.x = bit_range(halfword, 0, 7);
            win0_top_left.x  = bit_range(halfword, 8, 15);

            // garbage value
            if (win0_top_left.x > win0_bot_right.x ||
                win0_bot_right.x > display::LCD_WIDTH)
                win0_bot_right.x = display::LCD_WIDTH;
            break;
        }
        case WIN0V: {
            win0_bot_right.y = bit_range(halfword, 0, 7);
            win0_top_left.y  = bit_range(halfword, 8, 15);

            // garbage value
            if (win0_top_left.y > win0_bot_right.y ||
                win0_bot_right.y > display::LCD_HEIGHT)
                win0_bot_right.y = display::LCD_HEIGHT;
            break;
        }
        case WIN1H: {
            win1_bot_right.x = bit_range(halfword, 0, 7);
            win1_top_left.x  = bit_range(halfword, 8, 15);

            // garbage value
            if (win1_top_left.x > win1_bot_right.x ||
                win1_bot_right.x > display::LCD_WIDTH)
                win1_bot_right.x = display::LCD_WIDTH;
            break;
        }
        case WIN1V: {
            win1_bot_right.y = bit_range(halfword, 0, 7);
            win1_top_left.y  = bit_range(halfword, 8, 15);

            // garbage value
            if (win1_top_left.y > win1_bot_right.y ||
                win1_bot_right.y > display::LCD_HEIGHT)
                win1_bot_right.y = display::LCD_HEIGHT;
            break;
        }
        case WININ: {
            win1.write(bit_range(halfword, 8, 15));
            win0.write(bit_range(halfword, 0, 7));
            glogger.debug("writing winin {:b}", halfword);
            break;
        }
        case WINOUT: {
            win_obj.write(bit_range(halfword, 8, 15));
            win_out.write(bit_range(halfword, 0, 7));
            glogger.debug("writing winout {:b}", halfword);
            break;
        }
        case BLDCNT: {
            blend_control.write(halfword);
            break;
        }
        case BLDALPHA: {
            alpha_coeff.write(halfword);
            break;
        }
        case MOSAIC: {
            mosaic_size = halfword;
            break;
        }
        case BLDY: {
            brightness_coeff = halfword & 0b11111;
            break;
        }
        default: {
            glogger.warn("Invalid display IO address written at 0x{:08X}",
                         address);
        }
    }
}
}
}
