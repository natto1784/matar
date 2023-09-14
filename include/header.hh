#pragma once

#include <cstdint>
#include <string>

struct Header {
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
    std::string title;
    std::string title_code;
    UniqueCode unique_code;
    I18n i18n;
    uint8_t version;
    BootMode multiboot;
    uint32_t multiboot_entrypoint;
    uint8_t slave_id;
};
