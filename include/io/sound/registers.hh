#pragma once

#include <bit>
#include <cstdint>

namespace matar {
namespace sound {

static constexpr uint32_t FIFO_A = 0x40000A0;
static constexpr uint32_t FIFO_B = 0x40000A4;

static constexpr auto OUT_RES = 10;
static constexpr auto OUT_MAX = (1 << OUT_RES) - 1;
static constexpr auto OUT_TO_I16_MUL =
  std::numeric_limits<int16_t>::max() / ((1 << OUT_RES) / 2);
static constexpr auto DEFAULT_SAMPLING_RATE = 32768;

struct Ch1Sweep {
    struct {
        uint8_t shift : 3;
        uint8_t freq_dir : 1;
        uint8_t time : 3;
        int : 9; // unused
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct Ch1Envelope {
    struct {
        uint8_t sound_len : 6;
        uint8_t wave_duty : 2;
        uint8_t step_time : 3;
        uint8_t direction : 1;
        uint8_t init_vol : 4;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct Ch1FrequencyControl {
    struct {
        uint16_t frequency : 11;
        int : 3; // unused
        uint8_t len_flag : 1;
        uint8_t restart : 1;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

typedef Ch1Envelope Ch2Envelope;
typedef Ch1FrequencyControl Ch2FrequencyControl;

struct Ch3WaveSelect {
    struct {
        int : 5; // unused
        uint8_t dimension : 1;
        uint8_t bank_num : 1;
        uint8_t channel_on : 1;
        int : 8; // unused
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct Ch3LengthVolume {
    struct {
        uint8_t sound_len : 8;
        int : 5; // unused
        uint8_t volume : 2;
        uint8_t force_volume : 1;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

typedef Ch1FrequencyControl Ch3FrequencyControl;

struct Ch4Envelope {
    struct {
        uint8_t sound_len : 6;
        int : 2; // unused;
        uint8_t step_time : 3;
        uint8_t direction : 1;
        uint8_t init_vol : 4;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct Ch4FrequencyControl {
    struct {
        uint8_t div_ratio : 3;
        uint8_t counter_step : 1;
        uint8_t shift_freq : 4;
        int : 6; // unused
        uint8_t len_flag : 1;
        uint8_t restart : 1;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct LRVolumeControl {
    struct {
        uint8_t vol_right : 3;
        int : 1; // unused
        uint8_t vol_left : 3;
        int : 1; // unused
        bool ch_ena_right : 1;
        bool ch2_ena_right : 1;
        bool ch3_ena_right : 1;
        bool ch4_ena_right : 1;
        bool ch1_ena_left : 1;
        bool ch2_ena_left : 1;
        bool ch3_ena_left : 1;
        bool ch4_ena_left : 1;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct DmaControl {
    struct {
        uint8_t sound_1_4_volume : 2;
        uint8_t dma_a_volume : 1;
        uint8_t dma_b_volume : 1;
        int : 4; // unused
        bool dma_a_en_right : 1;
        bool dma_a_en_left : 1;
        bool dma_a_timer_sel : 1;
        bool dma_a_reset_fifo : 1;
        bool dma_b_en_right : 1;
        bool dma_b_en_left : 1;
        bool dma_b_timer_sel : 1;
        bool dma_b_reset_fifo : 1;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct SoundOnOff {
    struct {
        bool ch1_sound_on : 1;
        bool ch2_sound_on : 1;
        bool ch3_sound_on : 1;
        bool ch4_sound_on : 1;
        int : 3; // unused
        bool psg_fifo_master_ena : 1;
        int : 8; // unused
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};

struct SoundBias {
    struct {
        int : 1; // unused
        uint16_t level : 9;
        int : 4; // unused
        uint8_t amp_resolution : 2;
    } value;

    uint16_t read() const { return std::bit_cast<uint16_t>(value); };
    void write(uint16_t raw) { value = std::bit_cast<decltype(value)>(raw); };
};
}
}
