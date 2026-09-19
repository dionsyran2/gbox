#pragma once
#include <stdint.h>

#define TIMER_REG_DIV			0xFF04 // Divider Register
#define TIMER_REG_TIMA			0xFF05 // Timer Counter
#define TIMER_REG_TMA			0xFF06 // Timer Modulo
#define TIMER_REG_TAC			0xFF07 // Timer Control


struct gbtimer_t {
	int div_clock = 0;
	int tima_clock = 0;

	uint8_t div = 0;
	uint8_t tima = 0;
	uint8_t tma = 0;
	uint8_t tac = 0;

	void timer_tick(int cycles);
};

extern gbtimer_t timer;
