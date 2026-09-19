#pragma once
#include <stdint.h>
#include <atomic>
#include <mutex>

#define GBScreenWidth	160
#define GBScreenHeight	144

struct SharedState {
	std::atomic<bool> running{ true }; // When set to false the thread exits
	std::atomic<bool> paused{ true }; // Changes whether the emulation is paused or not
	std::atomic<bool> frame_ready{ false };
	std::atomic<bool> buttons_updated{ false };
	std::atomic<bool> can_save_cartridge{ false };

	std::atomic<bool> apu_ch1_enable{ true };
	std::atomic<bool> apu_ch2_enable{ true };
	std::atomic<bool> apu_ch3_enable{ true };
	std::atomic<bool> apu_ch4_enable{ true };


	/* Bit 0 : A */
	/* Bit 1 : B */
	/* Bit 2 : Select */
	/* Bit 3 : Start */
	/* Bit 4 : Right */
	/* Bit 5 : Left */
	/* Bit 6 : Up */
	/* Bit 7 : Down */
	std::atomic<uint8_t> button_states { 0 };

	std::mutex audio_mtx;
	float ch1_wave_history[512] = { 0 };
	int ch1_history_index = 0;

	float ch2_wave_history[512] = { 0 };
	int ch2_history_index = 0;

	float ch3_wave_history[512] = { 0 };
	int ch3_history_index = 0;

	float ch4_wave_history[512] = { 0 };
	int ch4_history_index = 0;

	float mixer_wave_history[512] = { 0 };
	int mixer_history_index = 0;


	std::mutex screen_mtx;
	uint32_t screen[GBScreenWidth * GBScreenHeight] = { 0 };
};


extern SharedState g_state;