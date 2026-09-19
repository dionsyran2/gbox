#pragma once
#include <stdint.h>

#define JOYPAD_REG			0xFF00


struct joypad_t {
	void handle_write(uint8_t data);
	uint8_t handle_read();
	void handle_update();

	uint8_t last_write = 0x30;
};


extern joypad_t joypad;