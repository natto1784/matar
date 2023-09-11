#pragma once

#include <cstdint>
#include <vector>

typedef struct {
    enum class UniqueCode {
        Old,        // old games
        New,        // new games
        Newer,      // unused (newer games)
        Famicom,    // NES
        YoshiKoro,  // acceleration sensor
        Ereader,    // dot code scanner
        Warioware,  // rumble and z-axis gyro
        Boktai,     // RTC and solar sensor
        DrillDozer, // rumble
    };

    enum class I18n {
        Japan,
        Europe,
        French,
        Spanish,
        Usa,
        German,
        Italian
    };

    enum class BootMode {
        Joybus,
        Normal,
        Multiplay
    };

    uint32_t entrypoint;
    char title[12];
    UniqueCode unique_code;
    char title_code[2];
    I18n i18n;
    uint8_t version;
    BootMode multiboot;
    uint32_t multiboot_entrypoint;
    uint8_t slave_id;
} Header;
