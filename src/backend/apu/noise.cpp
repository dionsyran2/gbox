#include <apu/noise.h>
#include <common.h>
#include <mutex>

void noise_channel_t::trigger() {
    if (this->length_counter == 0) {
        this->length_counter = 64;
    }

    this->envelope_timer = this->volume & 0x07;
    this->current_volume = (this->volume >> 4) & 0x0F;
    this->lfsr = 0x7FFF;
    
    this->dac_enabled = (this->volume & 0xF8) != 0;
    this->channel_enabled = this->dac_enabled;
}

void noise_channel_t::tick_length() {
    bool length_enabled = (this->period_hi_control & 0x40) != 0;
    
    if (length_enabled && this->length_counter > 0) {
        this->length_counter--;
        if (this->length_counter == 0) {
            this->channel_enabled = false;
        }
    }
}

void noise_channel_t::tick_envelope() {
    int env_pace = this->volume & 0x07;
    if (env_pace == 0) return; // Envelope disabled

    if (this->envelope_timer > 0) {
        this->envelope_timer--;
    }

    if (this->envelope_timer == 0) {
        this->envelope_timer = env_pace; // Reload timer

        bool env_increase = (this->volume & 0x08) != 0;
        if (env_increase && this->current_volume < 15) {
            this->current_volume++;
        } else if (!env_increase && this->current_volume > 0) {
            this->current_volume--;
        }
    }
}

void noise_channel_t::update(int cpu_cycles) {
    uint8_t divisor_code = this->lfsr_config & 0x07;
    uint8_t clock_shift = (this->lfsr_config >> 4) & 0x0F;
    bool lfsr_width = (this->lfsr_config & 0x08) != 0;

    int divisor = (divisor_code == 0) ? 8 : (divisor_code * 16);
    int cycles_per_step = divisor << clock_shift;

    if (cycles_per_step <= 0) return;

    this->cycle_accumulator += cpu_cycles;

    while (this->cycle_accumulator >= cycles_per_step) {
        this->cycle_accumulator -= cycles_per_step;

        uint8_t bit0 = this->lfsr & 1;
        uint8_t bit1 = (this->lfsr >> 1) & 1;
        uint8_t xor_result = bit0 ^ bit1;

        this->lfsr >>= 1;
        this->lfsr |= (xor_result << 14);

        if (lfsr_width) {
            this->lfsr &= ~(1 << 6);
            this->lfsr |= (xor_result << 6);
        }
    }
}

float noise_channel_t::get_output() {
    float r = 0.0f;
    if (this->channel_enabled && this->dac_enabled) {
        float raw_wave = ((this->lfsr & 1) == 0) ? 1.0f : -1.0f;
        r = raw_wave * ((float)this->current_volume / 15.0f);
    }  

    std::lock_guard<std::mutex> lock(g_state.audio_mtx);
    g_state.ch4_wave_history[g_state.ch4_history_index] = r;
    g_state.ch4_history_index = (g_state.ch4_history_index + 1) % 512;

    return r;
}

bool noise_channel_t::is_active() {
    return this->channel_enabled;
}