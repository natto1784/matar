#include "io/display/display.hh"

namespace matar {
namespace display {

/*
static constexpr uint LCD_HEIGHT = 160;
static constexpr uint LCD_WIDTH  = 240;
static constexpr uint BLANK      = 68;

static constexpr uint PIXEL_CYCLES    = 4;                             // 4
static constexpr uint HDRAW_CYCLES    = LCD_WIDTH * PIXEL_CYCLES + 46; // 1006
static constexpr uint HBLANK_CYCLES   = BLANK * PIXEL_CYCLES - 46;     // 226
static constexpr uint HREFRESH_CYCLES = HDRAW_CYCLES + HBLANK_CYCLES;  // 1232
static constexpr uint VDRAW_CYCLES    = LCD_HEIGHT * HREFRESH_CYCLES;  // 197120
static constexpr uint VBLANK_CYCLES   = BLANK * HREFRESH_CYCLES;       // 83776
static constexpr uint VREFRESH_CYCLES = VDRAW_CYCLES + VBLANK_CYCLES;  // 280896
*/

void
Display::mode_3() {}
}
}
