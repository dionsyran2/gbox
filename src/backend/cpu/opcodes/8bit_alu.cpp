#include <cpu/cpu.h>
#include <memory/memory.h>

uint8_t *get_register8(uint8_t y); // Defined in the misc.cpp file


int execute_addition(uint8_t opcode) {
    uint8_t source = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 4;

    if (source == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(source);
    }

    int result = value + cpu.registers.A;

    uint8_t a = cpu.registers.A;
    cpu.registers.A = (uint8_t)result;

    cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, ((a & 0x0F) + (value & 0x0F)) > 0x0F);
    cpu.registers.set_flag(F_CARRY, result > 0xFF);

    cpu.registers.PC += 1;
    return cycles;
}

int execute_addition_carry(uint8_t opcode) {
    uint8_t source = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 4;

    if (source == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(source);
    }

    uint8_t a = cpu.registers.A;
    uint8_t carry = cpu.registers.get_flag(F_CARRY) ? 1 : 0;

    int result = a + value + carry;
    int h_result = (a & 0x0F) + (value & 0x0F) + carry;

    cpu.registers.A = (uint8_t)result;

    cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, h_result > 0x0F);
    cpu.registers.set_flag(F_CARRY, result > 0xFF);

    cpu.registers.PC += 1;
    return cycles;
}

int execute_subtraction(uint8_t opcode) {
    uint8_t source = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 4;

    if (source == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(source);
    }

    uint8_t a = cpu.registers.A;
    int result = a - value;

    cpu.registers.A = (uint8_t)result;

    cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
    cpu.registers.set_flag(F_SUBTRACTION, true);
    cpu.registers.set_flag(F_HALF_CARRY, (a & 0x0F) < (value & 0x0F));
    cpu.registers.set_flag(F_CARRY, result < 0);

    cpu.registers.PC += 1;
    return cycles;
}

int execute_subtraction_carry(uint8_t opcode) {
    uint8_t source = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 4;

    if (source == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(source);
    }

    uint8_t a = cpu.registers.A;
    uint8_t carry = cpu.registers.get_flag(F_CARRY) ? 1 : 0;

    int result = a - value - carry;
    int h_result = (a & 0x0F) - (value & 0x0F) - carry;

    cpu.registers.A = (uint8_t)result;

    cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
    cpu.registers.set_flag(F_SUBTRACTION, true);
    cpu.registers.set_flag(F_HALF_CARRY, h_result < 0);
    cpu.registers.set_flag(F_CARRY, result < 0);

    cpu.registers.PC += 1;
    return cycles;
}

int execute_and(uint8_t opcode) {
    uint8_t source = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 4;

    if (source == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(source);
    }

    cpu.registers.A &= value;
    cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, true); // Hardware quirk: always 1
    cpu.registers.set_flag(F_CARRY, false);

    cpu.registers.PC += 1;
    return cycles;
}


int execute_xor(uint8_t opcode) {
    uint8_t source = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 4;

    if (source == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(source);
    }

    cpu.registers.A ^= value;
    cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, false);
    cpu.registers.set_flag(F_CARRY, false);

    cpu.registers.PC += 1;
    return cycles;
}

int execute_or(uint8_t opcode) {
    uint8_t source = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 4;

    if (source == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(source);
    }

    cpu.registers.A |= value;
    cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, false);
    cpu.registers.set_flag(F_CARRY, false);

    cpu.registers.PC += 1;
    return cycles;
}

int execute_cp(uint8_t opcode) {
    uint8_t source = opcode & 0b111;
    uint8_t value = 0;

    int cycles = 4;

    if (source == 6) {
        value = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        value = *get_register8(source);
    }

    uint8_t a = cpu.registers.A;
    int result = a - value;

    cpu.registers.set_flag(F_ZERO, (uint8_t)result == 0);
    cpu.registers.set_flag(F_SUBTRACTION, true);
    cpu.registers.set_flag(F_HALF_CARRY, (a & 0x0F) < (value & 0x0F));
    cpu.registers.set_flag(F_CARRY, result < 0);

    cpu.registers.PC += 1;
    return cycles;
}

int execute_8bit_alu(uint8_t opcode){
    uint8_t y = (opcode >> 3) & 0b111;

    switch (y) {
        case 0:
            return execute_addition(opcode);
        case 1:
            return execute_addition_carry(opcode);
        case 2:
            return execute_subtraction(opcode);
        case 3:
            return execute_subtraction_carry(opcode);
        case 4:
            return execute_and(opcode);
        case 5:
            return execute_xor(opcode);
        case 6:
            return execute_or(opcode);
        case 7:
            return execute_cp(opcode);
    }

    return 0;
}