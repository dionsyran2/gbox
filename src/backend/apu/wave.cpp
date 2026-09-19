#include <apu/wave.h>
#include <common.h>
#include <mutex>

void wave_channel_t::trigger() {
    if (this->length_counter == 0) {
        this->length_counter = 256;
    }
    this->current_wave_step = 0;
    
    bool dac = (this->dac_enable & 0x80) != 0;
    this->channel_enabled = dac;
}

void wave_channel_t::tick_length() {
    bool length_enabled = (this->period_hi_control & 0x40) != 0;
    
    if (length_enabled && this->length_counter > 0) {
        this->length_counter--;
        if (this->length_counter == 0) {
            this->channel_enabled = false;
        }
    }
}

void wave_channel_t::update(int cpu_cycles) {
    uint16_t period = this->period_lo | ((this->period_hi_control & 0b111) << 8);
    double cycles_per_step = 2.0 * (2048 - period);
    
    if (cycles_per_step <= 0) return;

    this->cycle_accumulator += cpu_cycles;

    while (this->cycle_accumulator >= cycles_per_step) {
        this->cycle_accumulator -= cycles_per_step;
        this->current_wave_step = (this->current_wave_step + 1) % 32;
    }
}

float wave_channel_t::get_output() {
    float r = 0.0f;
    bool dac = (this->dac_enable & 0x80) != 0;
    uint8_t volume_code = (this->volume >> 5) & 0x03;

    if (this->channel_enabled && dac && volume_code != 0) {
        uint8_t wave_byte = this->wave_ram[this->current_wave_step / 2];
        uint8_t raw_sample = (this->current_wave_step % 2 == 0) ? (wave_byte >> 4) : (wave_byte & 0x0F);

        uint8_t shifted_sample = 0;
        switch (volume_code) {
            case 1: shifted_sample = raw_sample;      break;
            case 2: shifted_sample = raw_sample >> 1; break;
            case 3: shifted_sample = raw_sample >> 2; break;
        }

        r = (shifted_sample / 7.5f) - 1.0f;
    }

    std::lock_guard<std::mutex> lock(g_state.audio_mtx);
    g_state.ch3_wave_history[g_state.ch3_history_index] = r;
    g_state.ch3_history_index = (g_state.ch3_history_index + 1) % 512;
    return r;
}

bool wave_channel_t::is_active() {
    return this->channel_enabled;
}