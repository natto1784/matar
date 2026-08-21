#pragma once

#include "io/dma/dma.hh"
#include "io/system/system.hh"
#include "memory.hh"
#include "registers.hh"
#include "scheduler.hh"

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
namespace matar {
namespace display {
class Display {
    using u16 = uint16_t;
    using u8  = uint8_t;

  public:
    Display(Scheduler& scheduler, System& system, Dma& dma);

    auto& get_pram() { return pram; }
    const auto& get_pram() const { return pram; }

    auto& get_vram() { return vram; }
    const auto& get_vram() const { return vram; }

    auto& get_oam() { return oam; }
    const auto& get_oam() const { return oam; }

    uint16_t read_halfword(uint32_t address) const;
    void write_halfword(uint32_t address, uint16_t value);

    void hblank_begin(uint64_t at);
    void hblank_end(uint64_t at);

    void vblank_begin();
    void vblank_end();

    size_t obj_offset() {
        if (lcd_control.value.mode >= 3) {
            return OBJ_START_BITMAP_MODE;
        }

        return OBJ_START_TEXT_MODE;
    }

  private:
    Scheduler& scheduler;
    System& system;
    Dma& dma;

    static constexpr uint32_t PRAM_SIZE = 0x400;
    static constexpr uint32_t VRAM_SIZE = 0x18000;
    static constexpr uint32_t OAM_SIZE  = 0x400;

    Memory<PRAM_SIZE> pram = {};
    Memory<VRAM_SIZE> vram = {};
    Memory<OAM_SIZE> oam   = {};

    /* registers */
    DisplayControl lcd_control;
    DisplayStatus lcd_status;
    u16 vertical_counter;
    BackgroundControl bg_control[4];
    Vec2<u16> bg_offset[4];
    RotationScaling bg2_rot_scale;
    RotationScaling bg3_rot_scale;
    Vec2<u8> win0_top_left;
    Vec2<u8> win0_bot_right;
    Vec2<u8> win1_top_left;
    Vec2<u8> win1_bot_right;
    WindowControl win0;
    WindowControl win1;
    WindowControl win_out;
    WindowControl win_obj;
    u16 mosaic_size;
    BlendControl blend_control;
    BlendAlpha alpha_coeff;
    u8 brightness_coeff;

    static constexpr uint32_t OBJ_START_BITMAP_MODE = 0x14000;
    static constexpr uint32_t OBJ_START_TEXT_MODE   = 0x10000;

    // 1 color is 16 bits in ARGB555 format
    std::array<std::array<Color, LCD_WIDTH>, N_BACKGROUNDS> scanline_buffers;
    std::array<std::array<ObjectPixel, LCD_HEIGHT>, LCD_WIDTH> object_buffer;
    std::array<uint16_t, LCD_WIDTH * LCD_HEIGHT> frame_buffer;

    uint8_t read_color_index(size_t address,
                             size_t x,
                             size_t y,
                             ColorDepth depth);
    Color fetch_color(uint32_t index, uint8_t bank, bool obj);

    template<int MODE,
             typename = std::enable_if_t<MODE == 3 || MODE == 4 || MODE == 5>>
    void render_bitmap_mode_line();

    template<int LAYER, typename = std::enable_if_t<LAYER >= 0 && LAYER <= 3>>
    void render_text_layer_line();

    template<int LAYER, typename = std::enable_if_t<LAYER == 2 || LAYER == 3>>
    void render_rot_scale_layer_line();

    void render_objects_line();

    void render_line();
};
}
}
// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
