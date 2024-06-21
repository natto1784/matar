#include "io/display/display.hh"

namespace matar {
namespace display {

struct TextScreen {
    uint16_t tile_number : 10;
    bool mirror_horizontal : 1;
    bool mirror_vertical : 1;
    uint8_t palette_number : 4;
};

// if 16th bit is set, this will denote the transparent color in rgb555 format
static constexpr uint16_t TRANSPARENT_RGB555 = 0x8000;

template<int MODE, typename>
void
Display::render_bitmap_mode() {
    static constexpr std::size_t VIEWPORT_WIDTH = MODE == 5 ? 160 : 240;

    for (int x = 0; x < LCD_WIDTH; x++) {
        // pixel to texel for x
        // shift by 8 cuz both ref.x and a are fixed point floats shifted by 8
        int32_t x_ = (bg2_rot_scale.ref.x + x * bg2_rot_scale.a) >> 8;
        int32_t y_ = (bg2_rot_scale.ref.y + x * bg2_rot_scale.c) >> 8;

        // ignore handling area overflow for bitmap modes
        // i am not sure how well this will turn out

        std::size_t idx = y_ * VIEWPORT_WIDTH + x_;

        // mode 3 and 5 takes 2 bytes per pixel
        if constexpr (MODE != 4)
            idx *= 2;

        // offset
        if constexpr (MODE != 3) {
            std::size_t offset =
              lcd_control.value.frame_select_1 ? 0xA000 : 0x0000;
            idx += offset;
        }

        // read two bytes
        if constexpr (MODE == 4)
            scanline_buffers[2][x] = pram.read_halfword(vram.read_byte(idx));
        else
            scanline_buffers[2][x] = vram.read_halfword(idx);
    }
}
}
}
