#pragma once

#include "memory.hh"

class Bus {
  public:
    Bus(Memory&& memory);

  private:
    Memory memory;
};
