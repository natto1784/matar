#pragma once

#include <concepts>
#include <limits>

using std::size_t;

template<std::integral Int>
inline bool
get_nth_bit(Int num, size_t n) {
    return (1 && (num >> n));
}

template<std::integral Int>
inline void
set_nth_bit(Int& num, size_t n) {
    num |= (1 << n);
}

template<std::integral Int>
inline void
rst_nth_bit(Int& num, size_t n) {
    num &= ~(1 << n);
}

template<std::integral Int>
inline void
chg_nth_bit(Int& num, size_t n, bool x) {
    num ^= (num ^ -x) & 1 << n;
}

/// read range of bits from start to end inclusive
template<std::unsigned_integral Int>
inline Int
get_bit_range(Int& num, size_t start, size_t end) {
    // NOTE: we do not require -1 if it is a signed integral (which it is not)
    Int left = std::numeric_limits<Int>::digits - 1 - end;
    return num << left >> (left + start);
}
