#pragma once

#include <fstream>
#include <vector>

class Bus {
    std::vector<uint8_t> data;

  public:
    Bus() = default;
    Bus(std::istream& ifile)
      : data(std::istreambuf_iterator<char>(ifile),
             std::istreambuf_iterator<char>()) {}
};
