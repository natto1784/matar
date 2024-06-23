#include "io/display/display.hh"
#include "io/display/registers.hh"
#include "io/dma/dma.hh"
#include "scheduler.hh"
#include "util/bits.hh"
#include "util/log.hh"
#include <algorithm>
#include <cassert>
#include <iostream>

namespace matar {
namespace display {

static constexpr uint32_t CYCLES_HDRAW  = 1006;
static constexpr uint32_t CYCLES_HBLANK = 226;

static constexpr uint32_t VDRAW_LINES  = LCD_HEIGHT;
static constexpr uint32_t VBLANK_LINES = 68;
uint64_t frames                        = 0;
static constexpr uint32_t VTOTAL_LINES = VDRAW_LINES + VBLANK_LINES;

Display::Display(Scheduler& scheduler, System& system, Dma& dma)
  : scheduler(scheduler)
  , system(system)
  , dma(dma) {
    scheduler.schedule_from_now(Task::Type::DISPLAY_HBLANK, CYCLES_HDRAW);
    scheduler.empty();
}

void
Display::hblank_begin(uint64_t at) {
    lcd_status.value.hblank_flag = true;

    /* within vdraw */
    if (vertical_counter < VDRAW_LINES) {
        if (lcd_status.value.hblank_irq_enable) {
            system.raise_irq(System::Irq::LCD_HBLANK);
        }

        dma.notify(DmaControl::Timing::HBlank, at);
    }

    scheduler.schedule_at(Task::Type::DISPLAY_HDRAW, at + CYCLES_HBLANK);
}

void
Display::hblank_end(uint64_t at) {
    vertical_counter++;

    lcd_status.value.hblank_flag = false;

    /* within vdraw */
    if (vertical_counter < VDRAW_LINES) {
        render_line();

        bg2_rot_scale.internal.x += bg2_rot_scale.b;
        bg2_rot_scale.internal.y += bg2_rot_scale.d;

        bg3_rot_scale.internal.x += bg3_rot_scale.b;
        bg3_rot_scale.internal.y += bg3_rot_scale.d;
    } else if (vertical_counter == VDRAW_LINES) {
        vblank_begin();
        dma.notify(DmaControl::Timing::VBlank, at);
    } else if (vertical_counter == VTOTAL_LINES) {
        vblank_end();
    }

    if (lcd_status.value.vcounter_irq_enable &&
        vertical_counter == lcd_status.value.vcount_setting) {
        system.raise_irq(System::Irq::LCD_VCOUNTER_MATCH);
    }

    scheduler.schedule_at(Task::Type::DISPLAY_HBLANK, at + CYCLES_HDRAW);
}

void
Display::vblank_begin() {
    lcd_status.value.vblank_flag = true;

    bg2_rot_scale.internal.x = bg2_rot_scale.ref.x;
    bg2_rot_scale.internal.y = bg2_rot_scale.ref.y;

    bg3_rot_scale.internal.x = bg3_rot_scale.ref.x;
    bg3_rot_scale.internal.y = bg3_rot_scale.ref.y;

    if (lcd_status.value.vblank_irq_enable) {
        system.raise_irq(System::Irq::LCD_VBLANK);
    }

    for (auto& row : object_buffer) {
        for (auto& pixel : row) {
            pixel.reset();
        }
    }
}

void
dump_frame(const uint16_t* src, FILE* f) {
    uint8_t rgb[240 * 160 * 3];

    for (int i = 0; i < 240 * 160; ++i) {
        uint16_t p = src[i];

        uint8_t r = (p >> 10) & 0x1f;
        uint8_t g = (p >> 5) & 0x1f;
        uint8_t b = (p >> 0) & 0x1f;

        // Expand 5-bit [0,31] to 8-bit [0,255]
        rgb[i * 3 + 0] = (r << 3) | (r >> 2);
        rgb[i * 3 + 1] = (g << 3) | (g >> 2);
        rgb[i * 3 + 2] = (b << 3) | (b >> 2);
    }

    fwrite(rgb, 1, sizeof(rgb), f);
}

void
Display::vblank_end() {
    lcd_status.value.vblank_flag = false;
    vertical_counter             = 0;

    render_line();

    if (lcd_status.value.vcounter_irq_enable &&
        lcd_status.value.vcount_setting == 0) {
        system.raise_irq(System::Irq::LCD_VCOUNTER_MATCH);
    }

    // std::cout << "frames: " << frames++ << std::endl;

    FILE* f = fopen("frames.raw", "ab");
    fwrite(frame_buffer.begin(), sizeof(uint16_t), 240 * 160, f);
    fclose(f);
}

void
Display::render_line() {
    uint y = vertical_counter;
    std::vector<uint8_t> bgs;

    if (lcd_control.value.forced_blank) {

        for (int x = 0; x < LCD_WIDTH; x++)
            frame_buffer[x + y * LCD_WIDTH] = 0xFFFF; // white

        return;
    }

    switch (lcd_control.value.mode) {
        case 0: {
            if (lcd_control.value.enable_bg_0) {
                render_text_layer_line<0>();
                bgs.push_back(0);
            }

            if (lcd_control.value.enable_bg_1) {
                render_text_layer_line<1>();
                bgs.push_back(1);
            }

            if (lcd_control.value.enable_bg_2) {
                render_text_layer_line<2>();
                bgs.push_back(2);
            }

            if (lcd_control.value.enable_bg_3) {
                render_text_layer_line<3>();
                bgs.push_back(3);
            }

            break;
        }
        case 1: {
            if (lcd_control.value.enable_bg_0) {
                render_text_layer_line<0>();
                bgs.push_back(0);
            }

            if (lcd_control.value.enable_bg_1) {
                render_text_layer_line<1>();
                bgs.push_back(1);
            }

            if (lcd_control.value.enable_bg_2) {
                render_rot_scale_layer_line<2>();
                bgs.push_back(2);
            }

            break;
        }

        case 2: {
            if (lcd_control.value.enable_bg_2) {
                render_rot_scale_layer_line<2>();
                bgs.push_back(2);
            }

            if (lcd_control.value.enable_bg_3) {
                render_rot_scale_layer_line<3>();
                bgs.push_back(3);
            }

            break;
        }
        case 3: {
            if (lcd_control.value.enable_bg_2) {
                render_bitmap_mode_line<3>();
                bgs.push_back(2);
            }
            break;
        }
        case 4: {
            if (lcd_control.value.enable_bg_2) {
                render_bitmap_mode_line<4>();
                bgs.push_back(2);
            }
            break;
        }
        case 5: {
            if (lcd_control.value.enable_bg_2) {
                render_bitmap_mode_line<5>();
                bgs.push_back(2);
            }
            break;
        }
        default: {
            // unreachable
        }
    }

    if (lcd_control.value.enable_obj)
        render_objects_line();

    std::ranges::stable_sort(bgs, [this](int a, int b) {
        return bg_control[a].value.priority < bg_control[b].value.priority;
    });

    bool y_in_win0 = y >= win0_top_left.y && y < win0_bot_right.y &&
                     lcd_control.value.window_display_0;
    bool y_in_win1 = y >= win1_top_left.y && y < win1_bot_right.y &&
                     lcd_control.value.window_display_1;

    uint16_t backdrop = pram.read_halfword(0);

    for (int x = 0; x < LCD_WIDTH; x++) {
        struct Pixel {
            Color color;
            uint8_t priority;
            bool blend;
        };

        enum SpecialEffects {
            None       = 0b00,
            AlphaBlend = 0b01,
            BrightInc  = 0b10,
            BrightDec  = 0b11
        };

        Pixel top    = { backdrop, 4, blend_control.top.backdrop },
              bottom = { backdrop, 4, blend_control.bottom.backdrop };

        bool top_found = false;

        WindowControl window;

        if (!lcd_control.value.window_display_0 &&
            !lcd_control.value.window_display_1 &&
            !lcd_control.value.obj_window_display) {
            window.write(0xff);
        } else if (y_in_win0 && x >= win0_top_left.x && x < win0_bot_right.x) {
            window = win0;
        } else if (y_in_win1 && x >= win1_top_left.x && x < win1_bot_right.x) {
            window = win1;
        } else if (lcd_control.value.obj_window_display &&
                   object_buffer[x][y].is_window) {
            window = win_obj;
        } else {
            window = win_out;
        }

        for (uint8_t bg : bgs) {
            if (!get_bit(window.value.bg_enable, bg))
                continue;

            if (scanline_buffers[bg][x].raw() == TRANSPARENT_RGB555) {
                continue;
            }

            if (!top_found) {
                top       = Pixel{ scanline_buffers[bg][x],
                             bg_control[bg].value.priority,
                             blend_control.bg(true, bg) };
                top_found = true;
            } else {
                bottom = Pixel{ scanline_buffers[bg][x],
                                bg_control[bg].value.priority,
                                blend_control.bg(false, bg) };

                break;
            }
        }
        bool obj_alpha     = false;
        ObjectPixel object = object_buffer[x][y];

        if (lcd_control.value.enable_obj && window.value.obj_enable &&
            object.color.raw() != TRANSPARENT_RGB555) {
            auto priority = object_buffer[x][y].priority;
            if (priority <= top.priority) {
                bottom = top;
                top    = Pixel{ object.color, priority, blend_control.top.obj };
                obj_alpha = object.is_alpha;
            } else if (priority <= bottom.priority)
                bottom =
                  Pixel{ object.color, priority, blend_control.bottom.obj };
        }

        size_t idx = y * LCD_WIDTH + x;

        SpecialEffects sfx = static_cast<SpecialEffects>(blend_control.top.sfx);

        if (window.value.special_effects && top.blend) {
            if (obj_alpha && bottom.blend) {
                glogger.error("WHAR");
                frame_buffer[idx] = top.color
                                      .blend(bottom.color,
                                             alpha_coeff.value.eva,
                                             alpha_coeff.value.evb)
                                      .raw();
            } else {
                switch (sfx) {
                    case SpecialEffects::AlphaBlend: {
                        if (bottom.blend) {
                            frame_buffer[idx] =
                              top.color
                                .blend(
                                  bottom.color,
                                  std::min<uint>(alpha_coeff.value.eva, 16),
                                  std::min<uint>(alpha_coeff.value.evb, 16))
                                .raw();
                            break;
                        }
                        [[fallthrough]];
                    }
                    case SpecialEffects::None: {
                        frame_buffer[idx] = top.color.raw();
                        break;
                    }
                    case SpecialEffects::BrightDec: {
                        auto evy = std::min<uint>(brightness_coeff, 16);
                        frame_buffer[idx] =
                          top.color.blend(Color(0), 16 - evy, evy).raw();
                        break;
                    }
                    case SpecialEffects::BrightInc: {
                        auto evy = std::min<uint>(brightness_coeff, 16);
                        frame_buffer[idx] =
                          top.color.blend(Color(0x7FFF), 16 - evy, evy).raw();
                        break;
                    }
                }
            }

        } else {
            frame_buffer[idx] = top.color.raw();
        }
    }
}

// if 16th bit is set, this will denote the transparent color in rgb555 format
uint8_t
Display::read_color_index(size_t address,
                          size_t x,
                          size_t y,
                          ColorDepth depth) {
    uint8_t color_index;

    if (depth == ColorDepth::BPP4) {
        address += ((y * 8) + x) / 2;
        color_index = vram.read_byte(address);

        if (x & 1)
            color_index >>= 4;
        else
            color_index &= 0xF;
    } else {
        address += (y * 8) + x;
        color_index = vram.read_byte(address);
    }

    return color_index;
}

    int read = false;
Color
Display::fetch_color(uint32_t index, uint8_t bank, bool obj) {
    if (index == 0 || (bank != 0 && index % 16 == 0)) {
        return Color(TRANSPARENT_RGB555);
    }

    uint32_t palette_addr = 2 * index + 0x20 * bank + (obj ? 0x200 : 0);
    return Color(pram.read_halfword(palette_addr) & 0x7FFF);
}
}
}
