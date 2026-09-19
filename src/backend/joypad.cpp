#include <joypad.h>
#include <common.h>
#include <memory/memory.h>
joypad_t joypad;


void joypad_t::handle_update() {
	// Called when the button_states are updated
	// Just fires the interrupt
	memory.bus_write(0xFF0F, memory.bus_read(0xFF0F) | (1 << 4));
}


void joypad_handle_write(uint16_t address, uint8_t data) {
	joypad.last_write = data;
}

uint8_t joypad_handle_read(uint16_t address) {
    // Start with all inputs unpressed
    uint8_t state = 0xF;

    // Action buttons
    if ((joypad.last_write & (1 << 5)) == 0) {
        state &= ~g_state.button_states.load(std::memory_order_relaxed);
    }

    // Direction buttons
    if ((joypad.last_write & (1 << 4)) == 0) {
        state &= ~(g_state.button_states.load(std::memory_order_relaxed) >> 4);
    }

    return (joypad.last_write & 0x30) | (state & 0xF) | 0xC0;
}

REGISTER_MEMORY_REGION(joypad_handle_read, joypad_handle_write, JOYPAD_REG, JOYPAD_REG);