#include <timer.h>
#include <memory/memory.h>

gbtimer_t timer;

void gbtimer_t::timer_tick(int cycles) {
	this->div_clock += cycles;

	if (this->div_clock >= 256) {
		this->div_clock -= 256;

		this->div++;
	}

	// Update the TIMA Register (0xFF05)
    if (this->tac & 0x04) {
        this->tima_clock += cycles;

        // Determine the frequency from Bits 0-1
        int thresholds[] = { 1024, 16, 64, 256 };
        int threshold = thresholds[this->tac & 0x03];

        if (this->tima_clock >= threshold) {
            this->tima_clock -= threshold;

            if (this->tima == 0xFF) {
                // Overflow! Reset to TMA
                this->tima = this->tma;

                // Request a Timer Interrupt (Bit 2 of IF register)
                uint8_t iff = memory.bus_read(0xFF0F);
                memory.bus_write(0xFF0F, iff | 0x04);
            }
            else {
                this->tima++;
            }
        }
    }
}

uint8_t timer_read(uint16_t address) {
    switch (address) {
        case TIMER_REG_DIV:
            return timer.div;
        case TIMER_REG_TIMA:
            return timer.tima;
        case TIMER_REG_TMA:
            return timer.tma;
        case TIMER_REG_TAC:
            return timer.tac;
    }

    return 0xFF;
}

void timer_write(uint16_t address, uint8_t value) {
    switch (address) {
        case TIMER_REG_DIV:
            timer.div = 0;
            timer.div_clock = 0; 
            break;
        case TIMER_REG_TIMA:
            timer.tima = value;
            break;
        case TIMER_REG_TMA:
            timer.tma = value;
            break;
        case TIMER_REG_TAC:
            timer.tac = value;
            break;
    }
}

REGISTER_MEMORY_REGION(timer_read, timer_write, TIMER_REG_DIV, TIMER_REG_TAC);