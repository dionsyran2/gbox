#pragma once
#include <stdint.h>
#include <memory/memory_region.h>

struct memory_t {
	//uint8_t vram[0x2000]; // 8KiB VRAM (Video RAM)
	uint8_t wram[0x2000]; // 8KiB WRAM (Work RAM)
	uint8_t hram[0x007F]; // High RAM? Not sure what this is about
	//uint8_t oam[0xA0]; // 160 bytes for Sprite data (0xFE00 - 0xFE9F)
	//uint8_t io[0x80];  // 128 bytes for Hardware I/O Registers (0xFF00 - 0xFF7F)

	//uint8_t interrupt_enable_register;

	uint8_t bus_read(uint16_t address);
	void bus_write(uint16_t address, uint8_t value);

	uint16_t bus_wread(uint16_t address);
	void bus_wwrite(uint16_t address, uint16_t value);
};

extern memory_t memory;

void push(uint16_t value);
uint16_t pop();