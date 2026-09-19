#pragma once
#include <stdint.h>

struct noise_channel_t {
    double cycle_accumulator = 0;
    uint16_t lfsr = 0x7FFF;

    /* REGISTERS */
    uint8_t length = 0;            // NR41 (0xFF20)
    uint8_t volume = 0;            // NR42 (0xFF21)
    uint8_t lfsr_config = 0;       // NR43 (0xFF22)
    uint8_t period_hi_control = 0; // NR44 (0xFF23)

    /* INTERNAL APU STATE */
    bool channel_enabled = false;
    bool dac_enabled = false;

    int length_counter = 0;
    int current_volume = 0;
    int envelope_timer = 0;

    /* FUNCTIONS */
    void trigger();
    void tick_length();
    void tick_envelope();
    void update(int cycles);
    bool is_active();
    float get_output();
};