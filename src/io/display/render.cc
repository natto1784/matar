#include "io/display/display.hh"
#include "io/display/registers.hh"
#include "util/bits.hh"
#include "util/log.hh"
#include <iostream>

namespace matar {
namespace display {

static constexpr uint32_t TILE_BLOCK_SIZE      = 16 * 1024; /* 16kb */
static constexpr uint32_t SCREEN_BLOCK_SIZE    = 2 * 1024;  /* 2kb */
static constexpr uint32_t TILE_SIZE_4BIT_DEPTH = 32;
static constexpr uint32_t TILE_SIZE_8BIT_DEPTH = 64;

template<std::integral Int>
static inline Vec2<Int>
pixel_to_texel(Vec2<Int> ref, Int x, Int a, Int c) {
    return { (ref.x + x * a) >> 8, (ref.y + x * c) >> 8 };
}

template<int MODE, typename>
void
Display::render_bitmap_mode_line() {
    static constexpr uint32_t VIEWPORT_WIDTH = MODE == 5 ? 160 : LCD_WIDTH;
    static constexpr uint32_t FRAME_1_OFFSET = 0xA000;

    for (auto x = 0; x < LCD_WIDTH; x++) {
        /* pixel to texel for x shift by 8 cuz both ref.x and a are fixed point
         * floats shifted by 8 terms with b and d are ignored cuz they are
         * already added at vblank to internal x and y */
        Vec2<int32_t> texel = pixel_to_texel<int32_t>(
          bg2_rot_scale.internal, x, bg2_rot_scale.a, bg2_rot_scale.c);

        uint32_t idx = texel.y * VIEWPORT_WIDTH + texel.x;

        /* mode 3 and 5 takes 2 bytes per pixel */
        if constexpr (MODE != 4) {
            idx *= 2;
        }

        /* offset */
        if constexpr (MODE != 3) {
            if (lcd_control.value.frame_select_1) {
                idx += FRAME_1_OFFSET;
            }
        }
        /* read two bytes */
        if constexpr (MODE == 4) {
            scanline_buffers[2][x] = fetch_color(vram.read_byte(idx), 0, 0);
        } else {
            scanline_buffers[2][x] = vram.read_halfword(idx);
        }
    }
}

/* explicit instantitation */
template void
Display::render_bitmap_mode_line<3>();
template void
Display::render_bitmap_mode_line<4>();
template void
Display::render_bitmap_mode_line<5>();

template<int LAYER, typename>
void
Display::render_text_layer_line() {
    constexpr uint32_t SCREEN_SIZE    = 256;
    constexpr uint32_t TILE_SIZE      = 8;
    constexpr uint32_t TILES_PER_ROW  = SCREEN_SIZE / TILE_SIZE;
    constexpr uint32_t MAP_ENTRY_SIZE = 2;

    const auto& control = bg_control[LAYER].value;

    const uint32_t tile_base = control.character_base_block * TILE_BLOCK_SIZE;
    const uint32_t map_base  = control.screen_base_block * SCREEN_BLOCK_SIZE;

    const bool color_256 = control.colors256;
    const uint32_t tile_data_size =
      color_256 ? TILE_SIZE_8BIT_DEPTH : TILE_SIZE_4BIT_DEPTH;

    Vec2<uint32_t> screen_pos{
        static_cast<uint32_t>(bg_offset[LAYER].x),
        static_cast<uint32_t>(bg_offset[LAYER].y) + vertical_counter,
    };
    uint32_t screen_index = 0;

    switch (control.screen_size) {
        case 0:
            /* 256 x 256 - SC0 */
            screen_pos.x %= 256;
            screen_pos.y %= 256;
            screen_index = 0;
            break;
        case 1:
            /* 512  x 256 - SC0,SC1 */
            screen_pos.x %= 512;
            screen_pos.y %= 256;
            screen_index = screen_pos.x / SCREEN_SIZE;
            break;
        case 2:
            /* 256 x 512 - SC0,SC1 */
            screen_pos.x %= 256;
            screen_pos.y %= 512;
            screen_index = screen_pos.y / SCREEN_SIZE;
            break;
        case 3:
            /* 512 x 512 - SC0,SC1,SC2,SC3 */
            screen_pos.x %= 512;
            screen_pos.y %= 512;
            screen_index =
              screen_pos.x / SCREEN_SIZE + (screen_pos.y / SCREEN_SIZE) * 2;
            break;
        default:
            std::unreachable();
    }

    /* every screen has 256x256 pixels, 32x32 tiles i.e, every tile has 64
     * pixels i.e, every row has 32 tiles and every tile row has 8 pixels */

    Vec2<uint32_t> tile_pos{
        (screen_pos.x % 256) / TILE_SIZE,
        (screen_pos.y % 256) / TILE_SIZE,
    };
    Vec2<uint32_t> pixel_in_tile{
        screen_pos.x % TILE_SIZE,
        screen_pos.y % TILE_SIZE,
    };

    int x = 0;

    while (x < LCD_WIDTH) {
        struct TextScreenMap {
            uint16_t tile_number : 10;
            bool mirrorx : 1;
            bool mirrory : 1;
            uint8_t palette_number : 4;
        };

        uint32_t map_index =
          map_base
          /* SC0,SC1,SC2,SC3 screen blocks are contiguous */
          + screen_index * SCREEN_BLOCK_SIZE
          /* Tile data is 2 bytes per entry */
          + (tile_pos.x + tile_pos.y * TILES_PER_ROW) * MAP_ENTRY_SIZE;

        TextScreenMap map =
          std::bit_cast<TextScreenMap>(vram.read_halfword(map_index));

        uint32_t tile_address = tile_base
                                /* 4bit depth -> 32 bytes per tile
                                 *  8bit depth -> 64 bytes per tile */
                                + map.tile_number * tile_data_size;

        while (pixel_in_tile.x < TILE_SIZE && x < LCD_WIDTH) {

            const uint8_t color_index = read_color_index(
              tile_address,
              map.mirrorx ? TILE_SIZE - 1 - pixel_in_tile.x : pixel_in_tile.x,
              map.mirrory ? TILE_SIZE - 1 - pixel_in_tile.y : pixel_in_tile.y,
              static_cast<ColorDepth>(color_256));

            scanline_buffers[LAYER][x] =
              fetch_color(color_index, color_256 ? 0 : map.palette_number, 0);

            pixel_in_tile.x++;
            x++;
        }

        pixel_in_tile.x = 0;

        /* should only happen once */
        if (++tile_pos.x == TILES_PER_ROW) {
            tile_pos.x = 0;

            if (control.screen_size == 1 || control.screen_size == 3) {
                screen_index ^= 1;
            }
        }
    }
}

/* explicit instantitation */
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
    constexpr auto TILE_SIZE = 8;

    uint32_t tile_base =
      bg_control[LAYER].value.character_base_block * TILE_BLOCK_SIZE;
    uint32_t map_base =
      bg_control[LAYER].value.screen_base_block * SCREEN_BLOCK_SIZE;
    int32_t screen_size = 128 << bg_control[LAYER].value.screen_size;

    const RotationScaling& rot_scale =
      LAYER == 2 ? bg2_rot_scale : bg3_rot_scale;

    for (int x = 0; x < LCD_WIDTH; x++) {
        /* pixel to texel for x shift by 8 cuz both ref.x and a are fixed point
         * floats shifted by 8 */
        Vec2<int32_t> texel = pixel_to_texel<int32_t>(
          rot_scale.internal, x, rot_scale.a, rot_scale.c);

        /* area overflow */
        if (texel.x < 0 || texel.x >= screen_size || texel.y < 0 ||
            texel.y >= screen_size) {
            if (bg_control[LAYER].value.bg_2_3_wraparound) {
                texel.x &= screen_size - 1;
                texel.y &= screen_size - 1;
            } else {
                scanline_buffers[LAYER][x] = fetch_color(0, 0, 0);
                continue;
            }
        }

        Vec2<uint32_t> tile_pos = {
            static_cast<uint32_t>(texel.x) / TILE_SIZE,
            static_cast<uint32_t>(texel.y) / TILE_SIZE,
        };

        auto n_tiles = screen_size / TILE_SIZE;

        auto map_address = map_base + (tile_pos.x + tile_pos.y * n_tiles);

        auto tile_address = tile_base + (uint32_t)vram.read_byte(map_address)
                                          /* only supports 8 bit depth */
                                          * TILE_SIZE_8BIT_DEPTH;

        uint8_t color_index = read_color_index(tile_address,
                                               texel.x % TILE_SIZE,
                                               texel.y % TILE_SIZE,
                                               ColorDepth::BPP8);

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

        Vec2<int32_t> coords = { o.a1.x, o.a0.y };
        ObjectMode mode      = static_cast<ObjectMode>(o.a0.mode);
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

            Vec2<int32_t> transformed;

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
