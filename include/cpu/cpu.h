#pragma once
#include <cpu/registers.h>

#define CPU_REG_PENDING_INTR	0xFF0F
#define CPU_REG_ENABLE_INTR		0xFFFF

struct cpu_t {
	registers_t registers;
	bool halted = false;

	uint8_t interrupts_enabled = 0;
	uint8_t interrupts_pending = 0;

	void reset();
	bool handle_interrupts();
	int load_opcode();
	int parse_opcode(uint8_t opcode);
	int step();
};

extern cpu_t cpu;