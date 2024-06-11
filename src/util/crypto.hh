#pragma once

#include <array>
#include <bit>
#include <format>
#include <string>

// Why I wrote this myself? I do not know
// I will off myself 😹😹😹😹
namespace crypto {

using std::rotr;

template<typename std::size_t N>
std::string
sha256(std::array<uint8_t, N>& data) {
    // Assuming 1 byte = 8 bits
    std::string string;
    size_t k = 512 - (N * 8 + 65) % 512;
    size_t L = N + (65 + k) / 8;
    size_t i = 0, j = 0;
    bool c = 0;

    static constexpr std::array<uint32_t, 64> K = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
        0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
        0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
        0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
        0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    std::array<uint32_t, 8> h = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372,
                                  0xa54ff53a, 0x510e527f, 0x9b05688c,
                                  0x1f83d9ab, 0x5be0cd19 };

    for (i = 0; i < L; i += 64) {
        size_t n = (N > i ? N - i : 0);

        std::array<uint32_t, 8> h0 = { 0 };
        std::array<uint32_t, 64> w = { 0 };

        if (n == 64)
            c = 1;

        if (n >= 64) {
            for (j = 0; j < 16; j++) {
                w[j] = data[i + j * 4] << 24 | data[i + j * 4 + 1] << 16 |
                       data[i + j * 4 + 2] << 8 | data[i + j * 4 + 3];
            }
        } else {
            std::array<uint32_t, 64> cur = { 0 };

            if (n) {
                std::copy(data.begin() + i, data.begin() + N, cur.begin());
                cur[n] = 0x80;
            } else if (c) {
                cur[n] = 0x80;
            }

            if (n < 54) {
                for (j = 56; j < 64; j++) {
                    cur[j] = (N * 8 >> ((63 - j) * 8)) & 0xFF;
                }
            }

            for (j = 0; j < 16; j++) {
                w[j] = cur[j * 4] << 24 | cur[j * 4 + 1] << 16 |
                       cur[j * 4 + 2] << 8 | cur[j * 4 + 3];
            }
        }

        for (j = 16; j < 64; j++) {
            uint32_t s0 =
              rotr(w[j - 15], 7) ^ rotr(w[j - 15], 18) ^ (w[j - 15] >> 3);
            uint32_t s1 =
              rotr(w[j - 2], 17) ^ rotr(w[j - 2], 19) ^ (w[j - 2] >> 10);

            w[j] = w[j - 16] + w[j - 7] + s0 + s1;
        }

        std::copy(h.begin(), h.end(), h0.begin());

        for (j = 0; j < 64; j++) {
            uint32_t s1  = rotr(h0[4], 6) ^ rotr(h0[4], 11) ^ rotr(h0[4], 25);
            uint32_t ch  = (h0[4] & h0[5]) ^ (~h0[4] & h0[6]);
            uint32_t t1  = h0[7] + s1 + ch + K[j] + w[j];
            uint32_t s0  = rotr(h0[0], 2) ^ rotr(h0[0], 13) ^ rotr(h0[0], 22);
            uint32_t maj = (h0[0] & h0[1]) ^ (h0[0] & h0[2]) ^ (h0[1] & h0[2]);
            uint32_t t2  = s0 + maj;

            h0[7] = h0[6];
            h0[6] = h0[5];
            h0[5] = h0[4];
            h0[4] = h0[3] + t1;
            h0[3] = h0[2];
            h0[2] = h0[1];
            h0[1] = h0[0];
            h0[0] = t1 + t2;
        }

        for (j = 0; j < 8; j++)
            h[j] += h0[j];
    }

    for (j = 0; j < 8; j++)
        for (i = 0; i < 4; i++)
            std::format_to(std::back_inserter(string),
                           "{:02x}",
                           ((h[j] >> (24 - i * 8)) & 0xFF));

    return string;
}
}
