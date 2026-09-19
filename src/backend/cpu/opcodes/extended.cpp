#include <cpu/cpu.h>
#include <memory/memory.h>

uint8_t *get_register8(uint8_t y); // Defined in misc.cpp

int handle_rlc(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    uint8_t carry_out = (value & 0x80) >> 7;

    uint8_t result = (value << 1) | carry_out;

    cpu.registers.set_flag(F_ZERO, result == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, false);
    cpu.registers.set_flag(F_CARRY, carry_out == 1);

    if (z == 6) {
        memory.bus_write(cpu.registers.HL, result);
        cycles += 4;
    } else {
        *get_register8(z) = result;
    }

    cpu.registers.PC += 1;
    return cycles;
}

int handle_rrc(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    uint8_t carry_out = value & 0x01; // Grab Bit 0

    // Shift right by 1, and put the old Bit 0 into Bit 7
    uint8_t result = (value >> 1) | (carry_out << 7);

    cpu.registers.set_flag(F_ZERO, result == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, false);
    cpu.registers.set_flag(F_CARRY, carry_out == 1);

    if (z == 6) {
        memory.bus_write(cpu.registers.HL, result);
        cycles += 4;
    } else {
        *get_register8(z) = result;
    }

    cpu.registers.PC += 1;
    return cycles;
}

int handle_rl(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    uint8_t carry_in = cpu.registers.get_flag(F_CARRY) ? 1 : 0;
    uint8_t carry_out = (value & 0x80) >> 7;

    uint8_t result = (value << 1) | carry_in;

    cpu.registers.set_flag(F_ZERO, result == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, false);
    cpu.registers.set_flag(F_CARRY, carry_out == 1);

    if (z == 6) {
        memory.bus_write(cpu.registers.HL, result);
        cycles += 4;
    } else {
        *get_register8(z) = result;
    }

    cpu.registers.PC += 1;
    return cycles;
}

int handle_rr(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    uint8_t carry_in = cpu.registers.get_flag(F_CARRY) ? 1 : 0;
    uint8_t carry_out = value & 0x01; // Grab Bit 0

    // Shift right by 1, and put the OLD carry into Bit 7
    uint8_t result = (value >> 1) | (carry_in << 7);

    cpu.registers.set_flag(F_ZERO, result == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, false);
    cpu.registers.set_flag(F_CARRY, carry_out == 1);

    if (z == 6) {
        memory.bus_write(cpu.registers.HL, result);
        cycles += 4;
    } else {
        *get_register8(z) = result;
    }

    cpu.registers.PC += 1;
    return cycles;
}

int handle_sla(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    uint8_t carry_out = (value & 0x80) >> 7;
    uint8_t result = value << 1;

    cpu.registers.set_flag(F_ZERO, result == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, false);
    cpu.registers.set_flag(F_CARRY, carry_out == 1);

    if (z == 6) {
        memory.bus_write(cpu.registers.HL, result);
        cycles += 4;
    } else {
        *get_register8(z) = result;
    }

    cpu.registers.PC += 1;
    return cycles;
}

int handle_sra(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    uint8_t carry_out = value & 0x01;
    uint8_t bit7 = value & 0x80; // Preserve the sign bit

    uint8_t result = (value >> 1) | bit7;

    cpu.registers.set_flag(F_ZERO, result == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, false);
    cpu.registers.set_flag(F_CARRY, carry_out == 1);

    if (z == 6) {
        memory.bus_write(cpu.registers.HL, result);
        cycles += 4;
    } else {
        *get_register8(z) = result;
    }

    cpu.registers.PC += 1;
    return cycles;
}

int handle_swap(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    uint8_t result = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);

    cpu.registers.set_flag(F_ZERO, result == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, false);
    cpu.registers.set_flag(F_CARRY, false);

    if (z == 6) {
        memory.bus_write(cpu.registers.HL, result);
        cycles += 4;
    } else {
        *get_register8(z) = result;
    }

    cpu.registers.PC += 1;
    return cycles;
}

int handle_srl(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    uint8_t carry_out = value & 0x01;
    uint8_t result = value >> 1; // Highest bit naturally fills with 0

    cpu.registers.set_flag(F_ZERO, result == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, false);
    cpu.registers.set_flag(F_CARRY, carry_out == 1);

    if (z == 6) {
        memory.bus_write(cpu.registers.HL, result);
        cycles += 4;
    } else {
        *get_register8(z) = result;
    }

    cpu.registers.PC += 1;
    return cycles;
}

int handle_x0(uint8_t ext_opcode) {
    uint8_t y = (ext_opcode >> 3) & 0b111;

    switch (y) {
        case 0:
            return handle_rlc(ext_opcode);
        case 1:
            return handle_rrc(ext_opcode);
        case 2:
            return handle_rl(ext_opcode);
        case 3:
            return handle_rr(ext_opcode);
        case 4:
            return handle_sla(ext_opcode);
        case 5:
            return handle_sra(ext_opcode);
        case 6:
            return handle_swap(ext_opcode);
        case 7:
            return handle_srl(ext_opcode);
    }

    return 0; // Unreachable
}

int handle_bit(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t bit = (opcode >> 3) & 0b111;

    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    bool is_set = (value & (1 << bit)) != 0;

    cpu.registers.set_flag(F_ZERO, !is_set);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, true);

    cpu.registers.PC += 1;
    return cycles;
}

int handle_res(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t bit = (opcode >> 3) & 0b111;

    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    uint8_t result = value & ~(1 << bit);

    if (z == 6) {
        memory.bus_write(cpu.registers.HL, result);
        cycles += 4;
    } else {
        *get_register8(z) = result;
    }

    cpu.registers.PC += 1;
    return cycles;
}

int handle_set(uint8_t opcode) {
    uint8_t z = opcode & 0b111;
    uint8_t bit = (opcode >> 3) & 0b111;

    uint8_t value = 0;

    int cycles = 8;

    if (z == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(z);
    }

    uint8_t result = value | (1 << bit);

    if (z == 6) {
        memory.bus_write(cpu.registers.HL, result);
        cycles += 4;
    } else {
        *get_register8(z) = result;
    }

    cpu.registers.PC += 1;
    return cycles;
}

int handle_prefix(uint8_t opcode) {

    cpu.registers.PC += 1;
    uint8_t ext_opcode = memory.bus_read(cpu.registers.PC);
    uint8_t x = ext_opcode >> 6;
    
    switch (x) {
        case 0:
            return handle_x0(ext_opcode);
        case 1:
            return handle_bit(ext_opcode);
        case 2:
            return handle_res(ext_opcode);
        case 3:
            return handle_set(ext_opcode);
    }

    return 0; // Unreachable
}