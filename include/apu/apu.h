#pragma once
#include <stdint.h>
#include <apu/square.h>
#include <apu/wave.h>
#include <apu/noise.h>

#define APU_REG_CH1_SWEEP       0xFF10
#define APU_REG_CH1_LENGTH      0xFF11 // Length & Duty
#define APU_REG_CH1_VOLUME      0xFF12
#define APU_REG_CH1_PERIOD_LO   0xFF13
#define APU_REG_CH1_PERIOD_CTRL 0xFF14

#define APU_REG_CH2_LENGTH      0xFF16 // Length & Duty
#define APU_REG_CH2_VOLUME      0xFF17
#define APU_REG_CH2_PERIOD_LO   0xFF18
#define APU_REG_CH2_PERIOD_CTRL 0xFF19

#define APU_REG_CH3_DAC         0xFF1A
#define APU_REG_CH3_LENGTH      0xFF1B
#define APU_REG_CH3_VOLUME      0xFF1C
#define APU_REG_CH3_PERIOD_LO   0xFF1D
#define APU_REG_CH3_PERIOD_CTRL 0xFF1E

#define APU_REG_CH4_LENGTH      0xFF20
#define APU_REG_CH4_VOLUME      0xFF21
#define APU_REG_CH4_LFSR        0xFF22
#define APU_REG_CH4_PERIOD_CTRL 0xFF23

#define APU_REG_AUDIO_MASTER		0xFF26
#define APU_REG_AUDIO_PANNING		0xFF25
#define APU_REG_MASTER_VOLUME		0xFF24


struct apu_t {
    /* Bit 0: CH1 Enable *
    * Bit 1: CH2 Enable *
    * Bit 2: CH3 Enable *
    * Bit 3: CH4 Enable *
    * Bit 7: APU Enable */
    uint8_t master_control = 0;

    /* Bit 0: CH1 Right  *
    * Bit 1: CH2 Right  * 
    * Bit 2: CH3 Right  *
    * Bit 3: CH4 Right  *
    * Bit 4: CH1 Left   *
    * Bit 5: CH2 Left   *
    * Bit 6: CH3 Left   *
    * Bit 7: CH4 Left   */
    uint8_t sound_panning = 0xFF;

    /* Bit 0 - 2: Right Volume *
    * Bit 3: VIN Right        *
    * Bit 4 - 6: Left Volume  *
    * Bit 7: VIN Left         */
    uint8_t master_volume = 0;

    double audio_cycle_accumulator = 0.0;
    double frame_sequencer_accumulator = 0.0;
    int frame_sequencer_step = 0;


    square_channel_t pulse1;
    square_channel_t pulse2;
    wave_channel_t wave;
    noise_channel_t noise;

    public:
	void initialize();
	void uninitialize();

	void step(int cycles);
	void push_sample(int16_t sample_left, int16_t sample_right);
    void mixer(float ch1, float ch2, float ch3, float ch4);
};

extern apu_t apu;