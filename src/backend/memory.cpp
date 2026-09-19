#include <memory/memory.h>
#include <stdio.h>
#include <list>
#include <cpu/cpu.h>

memory_t memory = { 0 };

mem_read_cb* get_read_map() {
    static mem_read_cb read_map[0x10000] = {nullptr};
    return read_map;
}

mem_write_cb* get_write_map() {
    static mem_write_cb write_map[0x10000] = {nullptr};
    return write_map;
}

mem_reg_int::mem_reg_int(mem_read_cb read, mem_write_cb write, uint16_t region_start, uint16_t region_end) {
    for (uint32_t i = region_start; i <= region_end; i++) {
        get_read_map()[i] = read;
        get_write_map()[i] = write;
    }
}

uint8_t memory_t::bus_read(uint16_t address) {
	if (address >= 0xC000 && address < 0xE000) {
		/* Work RAM */
		return this->wram[address - 0xC000];
	}
	else if (address >= 0xE000 && address < 0xFE00) {
		/* Echo RAM */
		return this->bus_read(address - 0x2000);
	}
	else if (address >= 0xFEA0 && address < 0xFF00) {
		/* Not Usable */
		return 0xFF;
	}
	else if (address >= 0xFF80 && address < 0xFFFF) {
		/* High RAM */
		return this->hram[address - 0xFF80];
	} else {
		mem_read_cb read = get_read_map()[address];
		if (read) {
			return read(address);
		}
	}

	return 0xFF;
}

void memory_t::bus_write(uint16_t address, uint8_t value) {
	if (address >= 0xC000 && address < 0xE000) {
		/* Work RAM */
		this->wram[address - 0xC000] = value;
	}
	else if (address >= 0xE000 && address < 0xFE00) {
		/* Echo RAM */
		this->bus_write(address - 0x2000, value); // I guess on h/w it would be wired directly to -0x2000
	}
	else if (address >= 0xFF80 && address < 0xFFFF) {
		/* High RAM */
		this->hram[address - 0xFF80] = value;
	} else {
		mem_write_cb write = get_write_map()[address];
		if (write) {
			return write(address, value);
		}
	}
}

uint16_t memory_t::bus_wread(uint16_t address) {
	uint8_t low = memory.bus_read(address);
	uint8_t high = memory.bus_read(address + 1);
	return ((uint16_t)high << 8) | low;
}

void memory_t::bus_wwrite(uint16_t address, uint16_t value) {
	uint8_t low = value & 0xFF;
	uint8_t high = (value >> 8) & 0xFF;

	memory.bus_write(address, low);
	memory.bus_write(address + 1, high);
}


void push(uint16_t value) {
	cpu.registers.SP -= sizeof(uint16_t);
	memory.bus_wwrite(cpu.registers.SP, value);
}

uint16_t pop() {
	uint16_t ret = memory.bus_wread(cpu.registers.SP);
	cpu.registers.SP += sizeof(uint16_t);

	return ret;
}
