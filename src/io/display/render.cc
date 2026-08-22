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

static constexpr auto OBJ_TILE_SIZE  = 32;
static constexpr auto OAM_ATTRS_SIZE = 6;
static constexpr auto OAM_GAP_SIZE   = 2;
static constexpr auto OAM_ENTRY_SIZE = OAM_ATTRS_SIZE + OAM_GAP_SIZE;

Display::OamAttributes
Display::read_oam_attributes(int idx) {
    size_t attr_address = OAM_ENTRY_SIZE * idx;

    OamAttributes o;

    o.attr0 = std::bit_cast<decltype(o.attr0)>(oam.read_halfword(attr_address));
    o.attr1 =
      std::bit_cast<decltype(o.attr1)>(oam.read_halfword(attr_address + 2));
    o.attr2 =
      std::bit_cast<decltype(o.attr2)>(oam.read_halfword(attr_address + 4));

    return o;
}

inline Vec2<int32_t>
Display::object_size(const OamAttributes& o) {
    static constexpr std::array<std::array<Vec2<int32_t>, 4>, 4> sizes{
        { /* Square*/
          { { { 8, 8 }, { 16, 16 }, { 32, 32 }, { 64, 64 } } },
          /* Horizontal */
          { { { 16, 8 }, { 32, 8 }, { 32, 16 }, { 64, 32 } } },
          /* Vertical */
          { { { 8, 16 }, { 8, 32 }, { 16, 32 }, { 32, 64 } } },
          /* Prohibited ??? */
          { { { 8, 8 }, { 8, 8 }, { 8, 8 }, { 8, 8 } } } }
    };

    return sizes[o.attr0.shape][o.attr1.size];
}

inline Display::RotationParams
Display::read_rotation_params(const OamAttributes& o) {
    static constexpr auto N_PARAMS = 4;
    RotationParams params;

    const size_t address =
      OAM_ATTRS_SIZE + o.attr1.trans_params * (N_PARAMS * OAM_ENTRY_SIZE);

    params.a =
      static_cast<int16_t>(oam.read_halfword(address + 0 * OAM_ENTRY_SIZE));
    params.b =
      static_cast<int16_t>(oam.read_halfword(address + 1 * OAM_ENTRY_SIZE));
    params.c =
      static_cast<int16_t>(oam.read_halfword(address + 2 * OAM_ENTRY_SIZE));
    params.d =
      static_cast<int16_t>(oam.read_halfword(address + 3 * OAM_ENTRY_SIZE));

    return params;
}

void
Display::render_one_object(int idx) {
    OamAttributes o    = read_oam_attributes(idx);
    uint32_t tile_base = OBJ_START_TEXT_MODE + o.attr2.number * OBJ_TILE_SIZE;
    ObjectMode mode    = static_cast<ObjectMode>(o.attr0.mode);
    Vec2<int32_t> size = object_size(o);
    Vec2<int32_t> display_size = size;
    Vec2<int32_t> obj_pos      = { o.attr1.x, o.attr0.y };
    RotationParams rot_params;

    if (obj_pos.x >= LCD_WIDTH)
        obj_pos.x -= 512;
    if (obj_pos.y >= LCD_HEIGHT)
        obj_pos.y -= 256;

    const auto y = vertical_counter;

    if (mode == ObjectMode::Prohibited) {
        return;
    }

    if (o.attr0.rot_scale_flag) {
        rot_params = read_rotation_params(o);

        if (o.attr0.double_size_or_obj_disable) {
            /* double size */
            display_size.x *= 2;
            display_size.y *= 2;
        }
    } else {
        if (o.attr0.double_size_or_obj_disable) {
            /* OBJ disable */
            return;
        }
    }

    /* continue if object does not cover current scanline */
    if (y < obj_pos.y || y >= obj_pos.y + display_size.y) {
        return;
    }

    /* ignore for bg mode 3-5 for numbers 0-511 */
    if (lcd_control.value.mode > 2 && o.attr2.number < 512) {
        return;
    }

    for (int x = obj_pos.x; x < obj_pos.x + display_size.x && x < LCD_WIDTH;
         x++) {
        if (x < 0) {
            continue;
        }

        if (object_buffer[x][y].priority <= o.attr2.priority &&
            o.attr0.mode != ObjectMode::Window) {
            continue;
        }

        Vec2<int32_t> tile_pos;

        if (o.attr0.rot_scale_flag) {
            /* if rotation/scaling is on */
            Vec2<int32_t> ref{
                x - obj_pos.x - display_size.x / 2,
                y - obj_pos.y - display_size.y / 2,
            };

            tile_pos = {
                ((rot_params.a * ref.x + rot_params.b * ref.y) >> 8) +
                  size.x / 2,
                ((rot_params.c * ref.x + rot_params.d * ref.y) >> 8) +
                  size.y / 2,
            };
            if (tile_pos.x < 0 || tile_pos.x >= size.x || tile_pos.y < 0 ||
                tile_pos.y >= size.y) {
                continue;
            }
        } else {
            tile_pos = { x - obj_pos.x, y - obj_pos.y };
            /* horizontal flip */
            if (get_bit(o.attr1.trans_params, 3)) {
                tile_pos.x = size.x - 1 - tile_pos.x;
            }
            /* vertical flip */
            if (get_bit(o.attr1.trans_params, 4)) {
                tile_pos.y = size.y - 1 - tile_pos.y;
            }
        }

        static constexpr auto TILE_SIZE     = 8;
        static constexpr auto VRAM_ROW_SIZE = 1024;

        uint8_t tile_size =
          o.attr0.colors256 ? TILE_SIZE_8BIT_DEPTH : TILE_SIZE_4BIT_DEPTH;

        size_t tiles_in_one_row = lcd_control.value.obj_vram_1d_mapping
                                    ? size.x / 8
                                    : VRAM_ROW_SIZE / tile_size;

        size_t tile_address =
          tile_base + tile_size * (tile_pos.x / TILE_SIZE +
                                   (tile_pos.y / TILE_SIZE) * tiles_in_one_row);

        uint8_t color_index =
          read_color_index(tile_address,
                           tile_pos.x % 8,
                           tile_pos.y % 8,
                           static_cast<ColorDepth>(o.attr0.colors256));

        Color color = fetch_color(
          color_index, (o.attr0.colors256 ? 0 : o.attr2.palette), 1);

        if (color.raw() == TRANSPARENT_RGB555) {
            continue;
        }

        if (mode == ObjectMode::Window) {
            object_buffer[x][y].is_window = true;
            continue;
        }

        object_buffer[x][y].color    = color;
        object_buffer[x][y].priority = o.attr2.priority;
        object_buffer[x][y].is_alpha = mode == ObjectMode::Alpha;
    }
}

void
Display::render_objects_line() {
    static constexpr auto N_OBJECTS = 128;

    for (int i = 0; i < N_OBJECTS; i++) {
        render_one_object(i);
    }
}
}
}
