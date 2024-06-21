#include "memory.hh"
#include <array>
#include <bit>
#include <cstdint>
#include <sys/types.h>

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
namespace matar {
namespace display {
static constexpr int LCD_WIDTH = 240;

// there are 5 modes
static constexpr uint N_MODES = 6;
// there are 4 backgrounds that can be layered depending on mode
// there is also 1 object layer
static constexpr uint N_BACKGROUNDS = 4;

static constexpr uint32_t PRAM_START = 0x5000000;
static constexpr uint32_t VRAM_START = 0x6000000;
static constexpr uint32_t OAM_START  = 0x7000000;

template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
struct Point {
    T x;
    T y;
};

struct Color {
  public:
    Color(uint16_t raw)
      : red(raw & 0b11111)
      , green(raw >> 5 & 0b11111)
      , blue(raw >> 10 & 0b11111) {}

    uint16_t read() const {
        return (red & 0b11111) | ((green << 5) & 0b11111) |
               ((blue << 10) & 0b11111);
    }

  private:
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct DisplayControl {
    struct {
        uint8_t mode : 3;
        int : 1; // unused
        bool frame_select_1 : 1;
        bool hblank_free_interval : 1;
        bool obj_character_vram_mapping : 1;
        bool forced_blank : 1;
        bool screen_display_0 : 1;
        bool screen_display_1 : 1;
        bool screen_display_2 : 1;
        bool screen_display_3 : 1;
        bool screen_display_obj : 1;
        bool window_display_0 : 1;
        bool window_display_1 : 1;
        bool obj_window_display : 1;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct DisplayStatus {
    struct {
        bool vblank_flag : 1;
        bool hblank_flag : 1;
        bool vcounter_flag : 1;
        bool vblank_irq_enable : 1;
        bool hblank_irq_enable : 1;
        bool vcounter_irq_enable : 1;
        int : 2; // unused
        uint8_t vcount_setting : 8;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct BackgroundControl {
    struct {
        uint8_t priority : 2;
        uint8_t character_base_block : 2;
        int : 2; // unused
        bool mosaic : 1;
        bool colors256 : 1;
        uint8_t screen_base_block : 5;
        bool bg_2_3_wraparound : 1;
        uint8_t screen_size : 2;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct RotationScaling {
    // these are all 16 bit signed "fixed point" floats
    // shifted by 8
    int16_t a;
    int16_t b;
    int16_t c;
    int16_t d;

    // following points have 28 bit signed "fixed point" floats as coords
    // shifted by 8
    Point<int32_t> ref;

  private:
    Point<int32_t> internal [[maybe_unused]]
    ;
};

struct Display {
  public:
    using u16 = uint16_t;

    Memory<0x400> pram;
    Memory<0x18000> vram;
    Memory<0x400> oam;

    DisplayControl lcd_control;
    DisplayStatus general_lcd_status;
    u16 vertical_counter;
    BackgroundControl bg_control[4];
    Point<u16> bg0_offset;
    Point<u16> bg1_offset;
    Point<u16> bg2_offset;
    Point<u16> bg3_offset;
    RotationScaling bg2_rot_scale;
    RotationScaling bg3_rot_scale;
    u16 win0_horizontal_dimensions;
    u16 win1_horizontal_dimensions;
    u16 win0_vertical_dimensions;
    u16 win1_vertical_dimensions;
    u16 inside_win_0_1;
    u16 outside_win;
    u16 mosaic_size;
    u16 color_special_effects_selection;
    u16 alpha_blending_coefficients;
    u16 brightness_coefficient;

  private:
    // 1 color is 16 bits in ARGB555 format
    std::array<std::array<uint16_t, LCD_WIDTH>, N_BACKGROUNDS> scanline_buffers;

    template<int MODE,
             typename = std::enable_if_t<MODE == 3 || MODE == 4 || MODE == 5>>
    void render_bitmap_mode();

    template<int LAYER, typename = std::enable_if_t<LAYER >= 0 && LAYER <= 3>>
    void render_text_layer();
};
}
}
// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
