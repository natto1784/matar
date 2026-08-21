#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <sys/types.h>

namespace matar {
namespace display {

static constexpr int LCD_WIDTH  = 240;
static constexpr int LCD_HEIGHT = 160;

// there are 5 modes
static constexpr uint N_MODES = 6;
// there are 4 backgrounds that can be layered depending on mode
// there is also 1 object layer
static constexpr uint N_BACKGROUNDS = 4;

template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
struct Vec2 {
    T x;
    T y;
};

enum ColorDepth {
    BPP4 = 0,
    BPP8 = 1
};

enum ObjectMode {
    Normal     = 0b00,
    Alpha      = 0b01,
    Window     = 0b10,
    Prohibited = 0b11
};

static constexpr uint16_t TRANSPARENT_RGB555 = 0x8000;
struct Color {
  public:
    Color()
      : Color(TRANSPARENT_RGB555) {}

    Color(uint16_t raw)
      : red(raw & 0b11111)
      , green(raw >> 5 & 0b11111)
      , blue(raw >> 10 & 0b11111)
      , alpha(raw >> 15) {}

    Color(uint8_t red, uint8_t green, uint8_t blue, bool alpha = false)
      : red(red)
      , green(green)
      , blue(blue)
      , alpha(alpha) {}

    uint16_t raw() const {
        return (red & 0b11111) | ((green & 0b11111) << 5) |
               ((blue & 0b11111) << 10) | (alpha << 15);
    }

    Color blend(Color o, uint8_t eva, uint8_t evb) {
        uint8_t red   = std::min(31, (this->red * eva + o.red * evb) / 16);
        uint8_t green = std::min(31, (this->green * eva + o.green * evb) / 16);
        uint8_t blue  = std::min(31, (this->blue * eva + o.blue * evb) / 16);

        return Color(red, green, blue);
    }

  private:
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    bool alpha;
};

struct ObjectPixel {
    uint8_t priority;
    Color color;
    bool is_window;
    bool is_alpha;

    ObjectPixel() { reset(); }

    void reset() {
        priority  = 4;
        color     = Color(TRANSPARENT_RGB555);
        is_window = false;
        is_alpha  = false;
    }
};

struct DisplayControl {
    struct {
        uint8_t mode : 3;
        int : 1; // unused
        bool frame_select_1 : 1;
        bool hblank_free_interval : 1;
        bool obj_vram_1d_mapping : 1;
        bool forced_blank : 1;
        bool enable_bg_0 : 1;
        bool enable_bg_1 : 1;
        bool enable_bg_2 : 1;
        bool enable_bg_3 : 1;
        bool enable_obj : 1;
        bool window_display_0 : 1;
        bool window_display_1 : 1;
        bool obj_window_display : 1;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };

    bool enable_bg(int bg) {
        switch (bg) {
            case 0:
                return value.enable_bg_0;
            case 1:
                return value.enable_bg_1;
            case 2:
                return value.enable_bg_2;
            case 3:
                return value.enable_bg_3;
        }

        return false;
    }
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

struct WindowControl {
    struct {
        uint8_t bg_enable : 4;
        bool obj_enable : 1;
        bool special_effects : 1;
        int : 2;
    } value;

    uint8_t read() const { return std::bit_cast<uint8_t>(value); };
    void write(uint8_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct BlendControl {
    struct Target {
        bool bg0 : 1;
        bool bg1 : 1;
        bool bg2 : 1;
        bool bg3 : 1;
        bool obj : 1;
        bool backdrop : 1;
        uint8_t sfx : 2;
    };

    Target top;
    Target bottom;

    uint16_t read() const {
        return std::bit_cast<uint8_t>(top) << 8 |
               std::bit_cast<uint8_t>(bottom);
    };
    void write(uint16_t raw) {
        top    = std::bit_cast<Target>(static_cast<uint8_t>((raw << 8) & 0xFF));
        bottom = std::bit_cast<Target>(static_cast<uint8_t>(raw & 0xFF));
    };

    bool bg(bool is_top, int bg) {
        Target target = is_top ? top : bottom;

        switch (bg) {
            case 0:
                return target.bg0;
            case 1:
                return target.bg1;
            case 2:
                return target.bg2;
            case 3:
                return target.bg3;
        }

        return false;
    }
};

struct BlendAlpha {
    struct {
        uint8_t eva : 5;
        int : 3;
        uint8_t evb : 5;
        int : 3;
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
    Vec2<int32_t> ref;
    Vec2<int32_t> internal;
};
}
}
