#include <apu/square.h>
#include <common.h>

static bool duty_table[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1}, // 12.5%
    {1, 0, 0, 0, 0, 0, 0, 1}, // 25%
    {1, 0, 0, 0, 0, 1, 1, 1}, // 50%
    {0, 1, 1, 1, 1, 1, 1, 0}  // 75%
};

void square_channel_t::trigger() {
    // Reset Length Counter if it reached 0
    if (this->length_counter == 0) {
        this->length_counter = 64;
    }

    // Reset Envelope
    this->envelope_timer = this->volume & 0x07;
    this->current_volume = (this->volume >> 4) & 0x0F;

    // Re-evaluate DAC & Channel Status
    this->dac_enabled = (this->volume & 0xF8) != 0;
    this->channel_enabled = this->dac_enabled;

    // Reset Sweep (Only if this is Channel 1)
    if (this->channel_has_sweep) {
        uint16_t period = this->period_lo | ((this->period_hi_control & 0b111) << 8);
        this->shadow_frequency = period;
        
        int sweep_period = (this->sweep >> 4) & 0x07;
        int sweep_shift = this->sweep & 0x07;
        
        this->sweep_timer = sweep_period > 0 ? sweep_period : 8;
        this->sweep_enabled = (sweep_period > 0) || (sweep_shift > 0);

        // Check for immediate overflow on trigger
        if (sweep_shift > 0) {
            uint16_t delta = this->shadow_frequency >> sweep_shift;
            bool sweep_negate = (this->sweep & 0x08) != 0;
            if (!sweep_negate && (this->shadow_frequency + delta > 2047)) {
                this->channel_enabled = false;
            }
        }
    }
}

void square_channel_t::tick_length() {
    bool length_enabled = (this->period_hi_control & 0x40) != 0;
    
    if (length_enabled && this->length_counter > 0) {
        this->length_counter--;
        if (this->length_counter == 0) {
            this->channel_enabled = false;
        }
    }
}

void square_channel_t::tick_envelope() {
    int env_pace = this->volume & 0x07;
    if (env_pace == 0) return; // Envelope disabled

    if (this->envelope_timer > 0) {
        this->envelope_timer--;
    }

    if (this->envelope_timer == 0) {
        this->envelope_timer = env_pace;

        bool env_increase = (this->volume & 0x08) != 0;
        if (env_increase && this->current_volume < 15) {
            this->current_volume++;
        } else if (!env_increase && this->current_volume > 0) {
            this->current_volume--;
        }
    }
}

void square_channel_t::tick_sweep() {
    if (!this->channel_has_sweep || !this->sweep_enabled) return;

    if (this->sweep_timer > 0) {
        this->sweep_timer--;
    }

    if (this->sweep_timer == 0) {
        int sweep_period = (this->sweep >> 4) & 0x07;
        int sweep_shift = this->sweep & 0x07;
        bool sweep_negate = (this->sweep & 0x08) != 0;

        this->sweep_timer = sweep_period > 0 ? sweep_period : 8;

        if (sweep_period > 0 && this->sweep_enabled) {
            uint16_t delta = this->shadow_frequency >> sweep_shift;
            uint16_t target = this->shadow_frequency;

            if (sweep_negate) {
                target -= delta;
            } else {
                target += delta;
            }

            if (target > 2047) {
                this->channel_enabled = false;
            } else if (sweep_shift > 0) {
                this->shadow_frequency = target;
                
                // Write the new frequency directly back into the raw registers
                this->period_lo = target & 0xFF;
                this->period_hi_control = (this->period_hi_control & ~0x07) | ((target >> 8) & 0x07);
            }
        }
    }
}

void square_channel_t::update(int cycles) {
    uint16_t period = this->period_lo | ((this->period_hi_control & 0b111) << 8);
    int wavelength_cycles = 32 * (2048 - period);
    
    if (wavelength_cycles <= 0) return;

    this->cycle_accumulator += cycles;
    double cycles_per_step = (double)wavelength_cycles / 8.0;

    while (this->cycle_accumulator >= cycles_per_step) {
        this->cycle_accumulator -= cycles_per_step;
        this->current_wave_step = (this->current_wave_step + 1) % 8;
    }
}

float square_channel_t::get_output() {
    float ret = 0.0f;

    // Only output sound if the channel is alive and the DAC has power
    if (this->channel_enabled && this->dac_enabled) {
        uint8_t duty_mode = (this->length_duty >> 6) & 0b11;
        float raw_wave = duty_table[duty_mode][this->current_wave_step] ? 1.0f : -1.0f;
        
        ret = raw_wave * ((float)this->current_volume / 15.0f);
    }

    std::lock_guard<std::mutex> lock(g_state.audio_mtx);
    if (this->secondary_channel) {
        g_state.ch2_wave_history[g_state.ch2_history_index] = ret;
        g_state.ch2_history_index = (g_state.ch2_history_index + 1) % 512;
    } else {
        g_state.ch1_wave_history[g_state.ch1_history_index] = ret;
        g_state.ch1_history_index = (g_state.ch1_history_index + 1) % 512;
    }

    return ret;
}

bool square_channel_t::is_active() {
    return this->channel_enabled;
}