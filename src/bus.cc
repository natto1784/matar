#include "bus.hh"

Bus::Bus(Memory&& memory)
  : memory(std::move(memory)) {}
