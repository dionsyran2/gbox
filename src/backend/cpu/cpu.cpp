#include <cpu/cpu.h>
#include <memory/memory.h>
#include <cstring>
#include <stdio.h>
#include <common.h>

cpu_t cpu;

uint16_t interrupt_service_routines[] = {
	0x0040, // ISR 0 (VBlank)
	0x0048, // ISR 1 (LCD STAT)
	0x0050, // ISR 2 (Timer)
	0x0058, // ISR 3 (Serial Link)
	0x0060, // ISR 4 (Joypad)
};

extern bool boot_rom_active;
void cpu_t::reset() {
	/*this->registers.AF = 0x01B0;
	this->registers.BC = 0x0013;
	this->registers.DE = 0x00D8;
	this->registers.HL = 0x014D;
	this->registers.SP = 0xFFFE;
	this->registers.PC = 0x100;*/
	boot_rom_active = true;
	this->registers.PC = 0x0000;
	this->registers.IME = false;
	this->halted = false;

	memset(memory.wram, 0, 0x2000);
	memset(memory.hram, 0, 0x7F);
}


int cpu_t::load_opcode() {
	uint8_t opcode = memory.bus_read(this->registers.PC);

	return this->parse_opcode(opcode);
}

int execute_misc_16bit(uint8_t opcode);
int execute_8bit_load(uint8_t opcode);
int execute_8bit_alu(uint8_t opcode);
int execute_branch_stack(uint8_t opcode);

int cpu_t::parse_opcode(uint8_t opcode) {
	// The opcode splits into:
	// x: bits 7-6, the main category
	// y: bits 5-3, the sub category
	// z: bits 2-0, the leaf

	// Main Categories:
	// 0: Misc & 16-bit Block
	// 1: 8-bit load block
	// 2: 8-bit alu math
	// 3: branching - stack & prefix

	uint8_t x = opcode >> 6;
	switch (x) {
		case 0:
			return execute_misc_16bit(opcode);
		case 1:
			return execute_8bit_load(opcode);
		case 2:
			return execute_8bit_alu(opcode);
		case 3:
			return execute_branch_stack(opcode);
		default:
			fprintf(stderr, "Illegal Instruction %.2x\n", opcode);
			g_state.paused.store(true);
			return 0;
	}
}

bool cpu_t::handle_interrupts() {
	uint8_t pending = this->interrupts_enabled & this->interrupts_pending & 0x1F;

	if (pending != 0) {
		this->halted = false;
	}

	if (this->registers.IME && pending != 0) {
		uint8_t irq = 0;

		// Get the first irq
		while (pending) {
			if (pending & 1) break;
			irq++;
			pending >>= 1;
		}

		this->registers.IME = false;
		this->interrupts_pending &= ~(1 << irq);
		
		push(this->registers.PC);
		cpu.registers.PC = interrupt_service_routines[irq];
		return true;
	}

	return false;
}

int cpu_t::step() {
	if (this->handle_interrupts()) {
		return 20; // Dispatching an interrupt takes exactly 20 cycles
	}

	if (this->halted) return 4;

	return load_opcode();
}

uint8_t cpu_mem_read(uint16_t address) {
	switch (address) {
		case CPU_REG_PENDING_INTR:
			return cpu.interrupts_pending;
		case CPU_REG_ENABLE_INTR:
			return cpu.interrupts_enabled;
	}

	return 0xFF;
}

void cpu_mem_write(uint16_t address, uint8_t value) {
	switch (address) {
		case CPU_REG_PENDING_INTR:
			cpu.interrupts_pending = value;
			break;
		case CPU_REG_ENABLE_INTR:
			cpu.interrupts_enabled = value;
			break;
	}
}

REGISTER_MEMORY_REGION(cpu_mem_read, cpu_mem_write, CPU_REG_PENDING_INTR, CPU_REG_PENDING_INTR); // Pending Interrupts
REGISTER_MEMORY_REGION(cpu_mem_read, cpu_mem_write, CPU_REG_ENABLE_INTR, CPU_REG_ENABLE_INTR); // Interrupt Enable