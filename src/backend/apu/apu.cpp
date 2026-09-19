#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include <apu/apu.h>
#include <memory/memory.h>
#include <mutex>
#include <queue>
#include <common.h>

apu_t apu;


std::mutex audio_mtx;
std::queue<int16_t> audio_queue;
ma_device audio_device;

constexpr double CYCLES_PER_SAMPLE = 4194304.0 / 44100.0;


void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    int16_t* out = (int16_t*)pOutput;
    std::lock_guard<std::mutex> lock(audio_mtx);

    if (audio_queue.size() < frameCount * 2) {
        for (ma_uint32 i = 0; i < frameCount * 2; ++i) {
            out[i] = 0;
        }
        return;
    }

    for (ma_uint32 i = 0; i < frameCount * 2; ++i) {
        out[i] = audio_queue.front();
        audio_queue.pop();
    }
}

void apu_t::initialize() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_s16;
    config.playback.channels = 2;
    config.sampleRate = 44100;
    config.dataCallback = audio_callback;

    if (ma_device_init(NULL, &config, &audio_device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to initialize audio device.\n");
        return;
    }
    ma_device_start(&audio_device);

    this->pulse1.channel_has_sweep = true;
    this->pulse2.secondary_channel = true;
}


void apu_t::uninitialize() {
    ma_device_uninit(&audio_device);
}

void apu_t::step(int cycles) {
    if ((this->master_control & (1 << 7)) == 0) {
        this->mixer(0, 0, 0, 0);
        return;
    }

    this->pulse1.update(cycles);
    this->pulse2.update(cycles);
    this->wave.update(cycles);
    this->noise.update(cycles);
    

    frame_sequencer_accumulator += cycles;
    if (frame_sequencer_accumulator >= 8192.0) {
        frame_sequencer_accumulator -= 8192.0;

        switch (frame_sequencer_step) {
        case 0:
            pulse1.tick_length();
            pulse2.tick_length();
            wave.tick_length();
            noise.tick_length();
            break;
        case 1:
            break;
        case 2:
            pulse1.tick_length();
            pulse1.tick_sweep();

            pulse2.tick_length();
            wave.tick_length();
            noise.tick_length();

            break;
        case 3:
            break;
        case 4:
            pulse1.tick_length();
            pulse2.tick_length();
            wave.tick_length();
            noise.tick_length();

            break;
        case 5:
            break;
        case 6:
            pulse1.tick_length();
            pulse1.tick_sweep();

            pulse2.tick_length();
            wave.tick_length();
            noise.tick_length();
            break;
        case 7:
            pulse1.tick_envelope();
            pulse2.tick_envelope();
            noise.tick_envelope();

            break;
        }

        // Advance the step loop (0 through 7)
        frame_sequencer_step = (frame_sequencer_step + 1) % 8;
    }

    audio_cycle_accumulator += cycles;

    if (audio_cycle_accumulator < CYCLES_PER_SAMPLE) return;
    audio_cycle_accumulator -= CYCLES_PER_SAMPLE;
    
    float sample1 = g_state.apu_ch1_enable.load() ? this->pulse1.get_output() : 0;
    float sample2 = g_state.apu_ch2_enable.load() ? this->pulse2.get_output() : 0;
    float sample3 = g_state.apu_ch3_enable.load() ? this->wave.get_output() : 0;
    float sample4 = g_state.apu_ch4_enable.load() ? this->noise.get_output() : 0;

    this->mixer(sample1, sample2, sample3, sample4);
}

void apu_t::mixer(float ch1, float ch2, float ch3, float ch4) {
    float left = 0;
    float right = 0;

    if (this->sound_panning & (1 << 0)) right += ch1;
    if (this->sound_panning & (1 << 1)) right += ch2;
    if (this->sound_panning & (1 << 2)) right += ch3;
    if (this->sound_panning & (1 << 3)) right += ch4;

    if (this->sound_panning & (1 << 4)) left += ch1;
    if (this->sound_panning & (1 << 5)) left += ch2;
    if (this->sound_panning & (1 << 6)) left += ch3;
    if (this->sound_panning & (1 << 7)) left += ch4;

    uint8_t rmultiplier = (this->master_volume & 0b111) + 1;
    uint8_t lmultiplier = ((this->master_volume >> 4) & 0b111) + 1;

    right *= rmultiplier;
    left *= lmultiplier;

    //Scale to 16-bit PCM (Max possible mix is 32.0f)
    int16_t lfinal = (int16_t)(left* 1024.0f);
    int16_t rfinal = (int16_t)(right* 1024.0f);
    this->push_sample(lfinal, rfinal);
}

void apu_t::push_sample(int16_t sample_left, int16_t sample_right) {
    std::lock_guard<std::mutex> lock(audio_mtx);

    if (audio_queue.size() < 4096) {
        audio_queue.push(sample_left);
        audio_queue.push(sample_right);
    }

    g_state.mixer_wave_history[g_state.mixer_history_index] = (float)sample_left / 32767.0f;
    g_state.mixer_history_index = (g_state.mixer_history_index + 1) % 512;
}

uint8_t apu_read(uint16_t address) {
    if (address >= 0xFF30 && address <= 0xFF3F) {
        return apu.wave.wave_ram[address - 0xFF30];
    }

    switch (address) {
        case APU_REG_AUDIO_MASTER: {
            uint8_t status = apu.master_control & 0x80;
            status |= 0x70; // Unused bits are always 1

            if (apu.pulse1.is_active()) status |= (1 << 0);
            if (apu.pulse2.is_active()) status |= (1 << 1);
            if (apu.wave.is_active())   status |= (1 << 2);
            if (apu.noise.is_active())  status |= (1 << 3);

            return status;
        }
        case APU_REG_AUDIO_PANNING:
            return apu.sound_panning;
        case APU_REG_MASTER_VOLUME:
            return apu.master_volume;

        // Channel 1
        case APU_REG_CH1_SWEEP:
            return apu.pulse1.sweep | 0x80; 
        case APU_REG_CH1_LENGTH:
            return (apu.pulse1.length_duty & 0xC0) | 0x3F; // Only Duty is readable
        case APU_REG_CH1_VOLUME:
            return apu.pulse1.volume;
        case APU_REG_CH1_PERIOD_LO:
            return 0xFF; // Write-only
        case APU_REG_CH1_PERIOD_CTRL:
            return (apu.pulse1.period_hi_control & 0x40) | 0xBF; // Only Length Enable readable

        // Channel 2
        case APU_REG_CH2_LENGTH:
            return (apu.pulse2.length_duty & 0xC0) | 0x3F;
        case APU_REG_CH2_VOLUME:
            return apu.pulse2.volume;
        case APU_REG_CH2_PERIOD_LO:
            return 0xFF;
        case APU_REG_CH2_PERIOD_CTRL:
            return (apu.pulse2.period_hi_control & 0x40) | 0xBF;

        // Channel 3 (Wave)
        case APU_REG_CH3_DAC:
            return (apu.wave.dac_enable & 0x80) | 0x7F;
        case APU_REG_CH3_LENGTH:
            return 0xFF; // Write-only
        case APU_REG_CH3_VOLUME:
            return (apu.wave.volume & 0x60) | 0x9F;
        case APU_REG_CH3_PERIOD_LO:
            return 0xFF;
        case APU_REG_CH3_PERIOD_CTRL:
            return (apu.wave.period_hi_control & 0x40) | 0xBF;

        // Channel 4 (Noise)
        case APU_REG_CH4_LENGTH: 
            return 0xFF; // Write-only
        case APU_REG_CH4_VOLUME:
            return apu.noise.volume;
        case APU_REG_CH4_LFSR:
            return apu.noise.lfsr_config;
        case APU_REG_CH4_PERIOD_CTRL:
            return (apu.noise.period_hi_control & 0x40) | 0xBF;
    }

    return 0xFF;
}


void apu_write(uint16_t address, uint8_t value) {
    // If the APU is turned off via master control, ignore all writes except to master control
    if ((apu.master_control & 0x80) == 0 && address != APU_REG_AUDIO_MASTER) {
        return;
    }

    if (address >= 0xFF30 && address <= 0xFF3F) {
        apu.wave.wave_ram[address - 0xFF30] = value;
        return;
    }

    switch (address) {
        case APU_REG_AUDIO_MASTER:
            apu.master_control = value & 0x80; // Only bit 7 is writable
            // Note: If bit 7 is set to 0, hardware actually zeroes out ALL APU registers here.
            break;
        case APU_REG_AUDIO_PANNING:
            apu.sound_panning = value;
            break;
        case APU_REG_MASTER_VOLUME:
            apu.master_volume = value;
            break;

        // Channel 1
        case APU_REG_CH1_SWEEP:
            apu.pulse1.sweep = value;
            break;
        case APU_REG_CH1_LENGTH:
            apu.pulse1.length_duty = value;
            apu.pulse1.length_counter = 64 - (value & 0x3F);
            break;
        case APU_REG_CH1_VOLUME:
            apu.pulse1.volume = value;
            break;
        case APU_REG_CH1_PERIOD_LO:
            apu.pulse1.period_lo = value;
            break;
        case APU_REG_CH1_PERIOD_CTRL:
            apu.pulse1.period_hi_control = value;
            if (value & 0x80) {
                apu.pulse1.trigger();
            }
            break;

        // Channel 2
        case APU_REG_CH2_LENGTH:
            apu.pulse2.length_duty = value;
            apu.pulse2.length_counter = 64 - (value & 0x3F);
            break;
        case APU_REG_CH2_VOLUME:
            apu.pulse2.volume = value;
            break;
        case APU_REG_CH2_PERIOD_LO:
            apu.pulse2.period_lo = value;
            break;
        case APU_REG_CH2_PERIOD_CTRL:
            apu.pulse2.period_hi_control = value;
            if (value & 0x80) {
                apu.pulse2.trigger();
            }
            break;

        // Channel 3 (Wave)
        case APU_REG_CH3_DAC: 
            apu.wave.dac_enable = value;
            break;
        case APU_REG_CH3_LENGTH:
            apu.wave.length = value;
            apu.wave.length_counter = 256 - value;
            break;
        case APU_REG_CH3_VOLUME:
            apu.wave.volume = value;
            break;
        case APU_REG_CH3_PERIOD_LO:
            apu.wave.period_lo = value;
            break;
        case APU_REG_CH3_PERIOD_CTRL:
            apu.wave.period_hi_control = value;
            if (value & 0x80) {
                apu.wave.trigger();
            }
            break;

        // Channel 4 (Noise)
        case APU_REG_CH4_LENGTH:
            apu.noise.length = value;
            apu.noise.length_counter = 64 - (value & 0x3F);
            break;
        case APU_REG_CH4_VOLUME:
            apu.noise.volume = value;
            break;
        case APU_REG_CH4_LFSR:
            apu.noise.lfsr_config = value;
            break;
        case APU_REG_CH4_PERIOD_CTRL:
            apu.noise.period_hi_control = value;
            if (value & 0x80) {
                apu.noise.trigger();
            }
            break;
    }
}


REGISTER_MEMORY_REGION(apu_read, apu_write, APU_REG_CH1_SWEEP, 0xFF3F);