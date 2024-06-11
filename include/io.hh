#pragma once
#include <cstdint>

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)

namespace matar {
class IoRegisters {
  public:
    uint8_t read(uint32_t) const;
    void write(uint32_t, uint8_t);

  private:
    struct {
        bool post_boot_flag           = false;
        bool interrupt_master_enabler = false;
    } system;
};
}

// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
