#pragma once
#include <stdint.h>

struct wave_channel_t {
    double cycle_accumulator = 0;
    uint8_t current_wave_step = 0;

    /* REGISTERS */
    uint8_t dac_enable = 0;        // NR30 (0xFF1A)
    uint8_t length = 0;            // NR31 (0xFF1B)
    uint8_t volume = 0;            // NR32 (0xFF1C)
    uint8_t period_lo = 0;         // NR33 (0xFF1D)
    uint8_t period_hi_control = 0; // NR34 (0xFF1E)
    
    uint8_t wave_ram[16] = {0};    // 0xFF30 - 0xFF3F

    /* INTERNAL APU STATE */
    bool channel_enabled = false;
    int length_counter = 0;

    /* FUNCTIONS */
    void trigger();
    void tick_length();
    void update(int cycles);
    bool is_active();
    float get_output();
};