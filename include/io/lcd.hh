#include <cstdint>

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)

/*
  4000000h  2    R/W  DISPCNT   LCD Control
  4000002h  2    R/W  -         Undocumented - Green Swap
  4000004h  2    R/W  DISPSTAT  General LCD Status (STAT,LYC)
  4000006h  2    R    VCOUNT    Vertical Counter (LY)
  4000008h  2    R/W  BG0CNT    BG0 Control
  400000Ah  2    R/W  BG1CNT    BG1 Control
  400000Ch  2    R/W  BG2CNT    BG2 Control
  400000Eh  2    R/W  BG3CNT    BG3 Control
  4000010h  2    W    BG0HOFS   BG0 X-Offset
  4000012h  2    W    BG0VOFS   BG0 Y-Offset
  4000014h  2    W    BG1HOFS   BG1 X-Offset
  4000016h  2    W    BG1VOFS   BG1 Y-Offset
  4000018h  2    W    BG2HOFS   BG2 X-Offset
  400001Ah  2    W    BG2VOFS   BG2 Y-Offset
  400001Ch  2    W    BG3HOFS   BG3 X-Offset
  400001Eh  2    W    BG3VOFS   BG3 Y-Offset
  4000020h  2    W    BG2PA     BG2 Rotation/Scaling Parameter A (dx)
  4000022h  2    W    BG2PB     BG2 Rotation/Scaling Parameter B (dmx)
  4000024h  2    W    BG2PC     BG2 Rotation/Scaling Parameter C (dy)
  4000026h  2    W    BG2PD     BG2 Rotation/Scaling Parameter D (dmy)
  4000028h  4    W    BG2X      BG2 Reference Point X-Coordinate
  400002Ch  4    W    BG2Y      BG2 Reference Point Y-Coordinate
  4000030h  2    W    BG3PA     BG3 Rotation/Scaling Parameter A (dx)
  4000032h  2    W    BG3PB     BG3 Rotation/Scaling Parameter B (dmx)
  4000034h  2    W    BG3PC     BG3 Rotation/Scaling Parameter C (dy)
  4000036h  2    W    BG3PD     BG3 Rotation/Scaling Parameter D (dmy)
  4000038h  4    W    BG3X      BG3 Reference Point X-Coordinate
  400003Ch  4    W    BG3Y      BG3 Reference Point Y-Coordinate
  4000040h  2    W    WIN0H     Window 0 Horizontal Dimensions
  4000042h  2    W    WIN1H     Window 1 Horizontal Dimensions
  4000044h  2    W    WIN0V     Window 0 Vertical Dimensions
  4000046h  2    W    WIN1V     Window 1 Vertical Dimensions
  4000048h  2    R/W  WININ     Inside of Window 0 and 1
  400004Ah  2    R/W  WINOUT    Inside of OBJ Window & Outside of Windows
  400004Ch  2    W    MOSAIC    Mosaic Size
  400004Eh       -    -         Not used
  4000050h  2    R/W  BLDCNT    Color Special Effects Selection
  4000052h  2    R/W  BLDALPHA  Alpha Blending Coefficients
  4000054h  2    W    BLDY      Brightness (Fade-In/Out) Coefficient
  4000056h       -    -         Not used
*/

struct lcd {
    using u16 = uint16_t;

    u16 lcd_control;
    u16 general_lcd_status;
    u16 vertical_counter;
    u16 bg0_control;
    u16 bg1_control;
    u16 bg2_control;
    u16 bg3_control;
    u16 bg0_x_offset;
    u16 bg0_y_offset;
    u16 bg1_x_offset;
    u16 bg1_y_offset;
    u16 bg2_x_offset;
    u16 bg2_y_offset;
    u16 bg3_x_offset;
    u16 bg3_y_offset;
    u16 bg2_rot_scaling_parameters[4];
    u16 bg2_reference_x[2];
    u16 bg2_reference_y[2];
    u16 bg3_rot_scaling_parameters[4];
    u16 bg3_reference_x[2];
    u16 bg3_reference_y[2];
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
};

// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
