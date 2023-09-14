#pragma once

#include <concepts>
#include <limits>

using std::size_t;

template<std::integral Int>
inline bool
get_bit(Int num, size_t n) {
    return (num >> n) & 1;
}

template<std::integral Int>
inline void
set_bit(Int& num, size_t n) {
    num |= (1 << n);
}

template<std::integral Int>
inline void
rst_bit(Int& num, size_t n) {
    num &= ~(1 << n);
}

template<std::integral Int>
inline void
chg_bit(Int& num, size_t n, bool x) {
    num = (num & ~(1 << n)) | (x << n);
}

/// read range of bits from start to end inclusive
template<std::integral Int>
inline Int
bit_range(Int num, size_t start, size_t end) {
    // NOTE: we do not require -1 if it is a signed integral
    Int left =
      std::numeric_limits<Int>::digits - (std::is_unsigned<Int>::value) - end;
    return num << left >> (left + start);
}
