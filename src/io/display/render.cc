#include "io/display/display.hh"
#include "io/display/registers.hh"
#include "util/bits.hh"
#include "util/log.hh"
#include <iostream>

namespace matar {
namespace display {

template<int MODE, typename>
void
Display::render_bitmap_mode_line() {
    static constexpr int VIEWPORT_WIDTH = MODE == 5 ? 160 : LCD_WIDTH;

    for (int x = 0; x < LCD_WIDTH; x++) {
        // pixel to texel for x
        // shift by 8 cuz both ref.x and a are fixed point floats shifted by 8
        // terms with b and d are ignored cuz they are already added at vblank
        // to internal x and y
        int32_t x_ = (bg2_rot_scale.internal.x + x * bg2_rot_scale.a) >> 8;
        int32_t y_ = (bg2_rot_scale.internal.y + x * bg2_rot_scale.c) >> 8;

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
        if constexpr (MODE == 4) {

            // Color color            = fetch_color(vram.read_byte(idx), 0, 0);
            scanline_buffers[2][x] = fetch_color(vram.read_byte(idx), 0, 0);
        } else
            scanline_buffers[2][x] = vram.read_halfword(idx);
    }
}

// explicit instantitation
template void
Display::render_bitmap_mode_line<3>();
template void
Display::render_bitmap_mode_line<4>();
template void
Display::render_bitmap_mode_line<5>();

template<int LAYER, typename>
void
Display::render_text_layer_line() {
    struct TextScreenMap {
        uint16_t tile_number : 10;
        bool mirror_x : 1;
        bool mirror_y : 1;
        uint8_t palette_number : 4;
    };
    uint32_t tile_base =
      bg_control[LAYER].value.character_base_block * 0x4000; // 16 kb
    uint32_t map_base =
      bg_control[LAYER].value.screen_base_block * 0x800; // 2 kb
    uint8_t screen_size_mode = bg_control[LAYER].value.screen_size;

    Point<int32_t> vp_coords;
    uint8_t screen_index;

    bool colors256    = bg_control[LAYER].value.colors256;
    uint8_t tile_size = colors256 ? 0x40 : 0x20;

    switch (screen_size_mode) {
            // 256 x 256 - SC0
        case 0: {
            vp_coords    = { bg_offset[LAYER].x % 256,
                             (vertical_counter + bg_offset[LAYER].y) % 256 };
            screen_index = 0;
            break;
        }
            // 512  x 256 - SC0,SC1
        case 1: {
            vp_coords    = { bg_offset[LAYER].x % 512,
                             (vertical_counter + bg_offset[LAYER].y) % 256 };
            screen_index = vp_coords.x / 256;
            break;
        }
            // 256 x 512 - SC0,SC1
        case 2: {
            vp_coords    = { bg_offset[LAYER].x % 256,
                             (vertical_counter + bg_offset[LAYER].y) % 512 };
            screen_index = vp_coords.y / 256;
            break;
        }

            // 512 x 512 - SC0,SC1,SC2,SC3
        case 3: {
            vp_coords    = { bg_offset[LAYER].x % 512,
                             (vertical_counter + bg_offset[LAYER].y) % 512 };
            screen_index = vp_coords.x / 256 + (vp_coords.y / 256) * 2;
            break;
        }
            // unreachable
        default: {
            glogger.error("this is NOT supposed to happen");
            std::abort();
        }
    }

    // every screen has 256x256 pixels, 32x32 tiles i.e, every tile has 64
    // pixels i.e, every row has 32 tiles and every tile row has 8 pixels

    Point<int32_t> tile = { (vp_coords.x % 256) / 8, (vp_coords.y % 256) / 8 };
    Point<int32_t> tile_pixel = { vp_coords.x % 8, vp_coords.y % 8 };

    for (int si = screen_index, x = 0; si <= screen_size_mode % 2; si++) {
        auto tx = tile.x;

        auto map_index = map_base;
        // starting address for every screen are separated at 2kb (0x800)
        map_index += 0x800 * si;
        // for "tx"th tile
        map_index += 2 * (tx + tile.y * 32);

        for (; tx < 32; tx++, map_index += 2) {
            auto tpx = tile_pixel.x;
            auto tpy = tile_pixel.y;

            uint16_t map_raw  = vram.read_halfword(map_index);
            TextScreenMap map = std::bit_cast<TextScreenMap>(map_raw);

            auto tile_address = tile_base;
            // get the ith tile
            tile_address += tile_size * map.tile_number;

            for (; tpx < 8; tpx++) {
                auto tpx_ = map.mirror_x ? 7 - tpx : tpx;
                auto tpy_ = map.mirror_y ? 7 - tpy : tpy;

                uint8_t color_index = read_color_index(
                  tile_address, tpx_, tpy_, static_cast<ColorDepth>(colors256));
                Color color = fetch_color(
                  color_index, colors256 ? 0 : map.palette_number, 0);

                scanline_buffers[LAYER][x] = color;

                if (++x == LCD_WIDTH)
                    goto BREAK;
            }
            tile_pixel.x = 0;
        }
        tile.x = 0;
    }

BREAK:
}

// explicit instantitation
template void
Display::render_text_layer_line<0>();
template void
Display::render_text_layer_line<1>();
template void
Display::render_text_layer_line<2>();
template void
Display::render_text_layer_line<3>();

template<int LAYER, typename>
void
Display::render_rot_scale_layer_line() {
    uint32_t tile_base =
      bg_control[LAYER].value.character_base_block * 0x4000; // 16 kb
    uint32_t map_base =
      bg_control[LAYER].value.screen_base_block * 0x800; // 2 kb
    int32_t screen_size = 128 << bg_control[LAYER].value.screen_size;

    for (int x = 0; x < LCD_WIDTH; x++) {
        RotationScaling rot_scale;

        if constexpr (LAYER == 2) {
            rot_scale = bg2_rot_scale;
        } else {
            rot_scale = bg3_rot_scale;
        }
        // pixel to texel for x
        // shift by 8 cuz both ref.x and a are fixed point floats shifted by 8
        int32_t x_ = (rot_scale.internal.x + x * rot_scale.a) >> 8;
        int32_t y_ = (rot_scale.internal.y + x * rot_scale.c) >> 8;

        // area overflow
        if (x_ < 0 || x_ >= screen_size || y_ < 0 || y_ >= screen_size) {
            if (bg_control[LAYER].value.bg_2_3_wraparound) {
                x_ &= screen_size - 1;
                y_ &= screen_size - 1;
            } else {
                scanline_buffers[LAYER][x] = fetch_color(0, 0, 0);
                continue;
            }
        }

        Point<int32_t> tile = { x_ / 8, y_ / 8 }; // each tile is 8x8 pixels
        auto n_tiles        = screen_size / 8;

        auto map_address = map_base + (tile.x + tile.y * n_tiles);

        auto tile_address =
          tile_base + (uint32_t)vram.read_byte(map_address) * 0x40; // bpp8 only

        uint8_t color_index =
          read_color_index(tile_address, x_ % 8, y_ % 8, ColorDepth::BPP8);

        scanline_buffers[LAYER][x] = fetch_color(color_index, 0, 0);
    }
}

// explicit instantitation
template void
Display::render_rot_scale_layer_line<2>();
template void
Display::render_rot_scale_layer_line<3>();

void
Display::render_objects_line() {
    struct OamAttributes {
        struct {
            int8_t y : 8;
            uint8_t rot_scale : 2;
            uint8_t mode : 2;
            bool mosaic : 1;
            bool colors256 : 1;
            uint8_t shape : 2;
        } a0;

        struct {
            int16_t x : 9;
            uint8_t trans_params : 5;
            uint8_t size : 2;
        } a1;

        struct {
            uint16_t number : 10;
            uint8_t priority : 2;
            uint8_t palette : 4;
        } a2;
    };

    for (int i = 0; i < 128; i++) {
        size_t address = 8 * i;
        OamAttributes o;
        o.a0 = std::bit_cast<decltype(o.a0)>(oam.read_halfword(address));
        o.a1 = std::bit_cast<decltype(o.a1)>(oam.read_halfword(address + 2));
        o.a2 = std::bit_cast<decltype(o.a2)>(oam.read_halfword(address + 4));

        Point<int32_t> coords = { o.a1.x, o.a0.y };
        ObjectMode mode       = static_cast<ObjectMode>(o.a0.mode);
        if (coords.x >= LCD_WIDTH)
            coords.x -= 512;
        if (coords.y >= LCD_HEIGHT)
            coords.y -= 256;

        uint32_t tile_base = 0x10000 + o.a2.number * 0x20;

        static constexpr std::array<std::array<std::pair<int32_t, int32_t>, 4>,
                                    4>
          OBJ_SIZES{ { // Square
                       { { { 8, 8 }, { 16, 16 }, { 32, 32 }, { 64, 64 } } },
                       // Horizontal
                       { { { 16, 8 }, { 32, 8 }, { 32, 16 }, { 64, 32 } } },
                       // Vertical
                       { { { 8, 16 }, { 8, 32 }, { 16, 32 }, { 32, 64 } } },
                       // Prohibited ???
                       { { { 8, 8 }, { 8, 8 }, { 8, 8 }, { 8, 8 } } } } };

        auto [size_x, size_y] = OBJ_SIZES[o.a0.shape][o.a1.size];

        // this is the display size, which is doubled if double bit is set
        auto dsize_x = size_x;
        auto dsize_y = size_y;

        // double sized
        if (o.a0.rot_scale == 0b11)
            dsize_x *= 2, dsize_y *= 2;

        const auto y = vertical_counter;

        // OBJ disable
        if (o.a0.rot_scale == 0b10)
            continue;

        if (mode == ObjectMode::Prohibited)
            continue;

        // continue if object does not cover current scanline
        if (y < coords.y || y >= coords.y + dsize_y)
            continue;

        // ignore for bg mode 3-5 for numbers 0-511
        if (lcd_control.value.mode > 2 && o.a2.number < 512)
            continue;

        // coords.x max value is 255

        for (int x = coords.x; x < coords.x + dsize_x && x < LCD_WIDTH; x++) {

            if (x < 0)
                continue;

            if (object_buffer[x][y].priority <= o.a2.priority &&
                o.a0.mode != ObjectMode::Window) {
                continue;
            }

            Point<int32_t> transformed;

            // im gonna trust the -O3 and pray this gets optimised
            if (get_bit(o.a0.rot_scale, 0)) {
                // if rotation/scaling is on
                size_t param_offset = o.a1.trans_params * 0x20 + 6;
                int32_t a =
                  static_cast<int16_t>(oam.read_halfword(param_offset));
                int32_t b =
                  static_cast<int16_t>(oam.read_halfword(param_offset + 8));
                int32_t c =
                  static_cast<int16_t>(oam.read_halfword(param_offset + 16));
                int32_t d =
                  static_cast<int16_t>(oam.read_halfword(param_offset + 24));

                int32_t ref_x = x - coords.x - dsize_x / 2;
                int32_t ref_y = y - coords.y - dsize_y / 2;

                transformed = { ((a * ref_x + b * ref_y) >> 8) + size_x / 2,
                                ((c * ref_x + d * ref_y) >> 8) + size_y / 2 };
                if (transformed.x < 0 || transformed.x >= size_x ||
                    transformed.y < 0 || transformed.y >= size_y) {
                    continue;
                }
            } else {
                transformed = { x - coords.x, y - coords.y };
                // horizontal flip
                if (get_bit(o.a1.trans_params, 3)) {
                    transformed.x = size_x - 1 - transformed.x;
                }
                // vertical flip
                if (get_bit(o.a1.trans_params, 4)) {
                    transformed.y = size_y - 1 - transformed.y;
                }
            }

            uint8_t tile_size = o.a0.colors256 ? 0x40 : 0x20;

            size_t tile_address =
              tile_base +
              tile_size *
                (transformed.x / 8 + (transformed.y / 8) *
                                       // how many tile addresses are in one row
                                       (lcd_control.value.obj_vram_1d_mapping
                                          ? size_x / 8
                                          : (o.a0.colors256 ? 16 : 32)));

            uint8_t color_index =
              read_color_index(tile_address,
                               transformed.x % 8,
                               transformed.y % 8,
                               static_cast<ColorDepth>(o.a0.colors256));

            Color color =
              fetch_color(color_index, (o.a0.colors256 ? 0 : o.a2.palette), 1);

            if (color.raw() == TRANSPARENT_RGB555) {
                continue;
            }

            if (mode == ObjectMode::Window) {
                object_buffer[x][y].is_window = true;
                continue;
            }

            object_buffer[x][y].color    = color;
            object_buffer[x][y].priority = o.a2.priority;
            object_buffer[x][y].is_alpha = mode == ObjectMode::Alpha;
        }
    }
}
}
}
