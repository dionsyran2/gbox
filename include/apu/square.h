#pragma once
#include <stdint.h>

struct square_channel_t {
    double cycle_accumulator = 0;
    uint8_t current_wave_step = 0;

    bool secondary_channel = false; // Set to true for Channel 2
    bool channel_has_sweep = false; // Set to true for Channel 1

    /* REGISTERS */
    uint8_t sweep = 0;             // NR10
    uint8_t length_duty = 0;       // NR11 / NR21
    uint8_t volume = 0;            // NR12 / NR22
    uint8_t period_lo = 0;         // NR13 / NR23
    uint8_t period_hi_control = 0; // NR14 / NR24

    /* INTERNAL APU STATE */
    bool channel_enabled = false;
    bool dac_enabled = false;
    
    int length_counter = 0;
    
    int current_volume = 0;
    int envelope_timer = 0;
    
    int sweep_timer = 0;
    uint16_t shadow_frequency = 0;
    bool sweep_enabled = false;

    /* FUNCTIONS */
    void trigger();
    void tick_length();
    void tick_envelope();
    void tick_sweep();
    void update(int cycles);
    bool is_active();
    float get_output();
};