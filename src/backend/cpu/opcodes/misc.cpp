#include <cpu/cpu.h>
#include <memory/memory.h>

uint16_t *get_register16(uint8_t p) {
    uint16_t *reg = &cpu.registers.SP;
    switch (p) {
        case 0b00:
            reg = &cpu.registers.BC;
            break;
        case 0b01:
            reg = &cpu.registers.DE;
            break;
        case 0b10:
            reg = &cpu.registers.HL;
            break;
    }
    return reg;
}

uint8_t *get_register8(uint8_t y) {
    uint8_t *reg = &cpu.registers.A;

    switch (y) {
        case 0:
            reg = &cpu.registers.B;
            break;
        case 1:
            reg = &cpu.registers.C;
            break;
        case 2:
            reg = &cpu.registers.D;
            break;
        case 3:
            reg = &cpu.registers.E;
            break;
        case 4:
            reg = &cpu.registers.H;
            break;
        case 5:
            reg = &cpu.registers.L;
            break;
    }

    return reg;
}

int execute_ld16(uint8_t opcode){
    uint8_t p = (opcode >> 4) & 0b11;

    // 00: BC, n16
    // 01: DE, n16
    // 10: HL, n16
    // 11: SP, n16
    uint16_t *reg = get_register16(p);

    uint16_t immediate = memory.bus_wread(cpu.registers.PC + 1);
    *reg = immediate;

    cpu.registers.PC += 3;
    return 12;
}

int execute_add16(uint8_t opcode){
    uint8_t p = (opcode >> 4) & 0b11;

    // 00: BC, n16
    // 01: DE, n16
    // 10: HL, n16
    // 11: SP, n16
    uint16_t *reg = &cpu.registers.SP;

    switch (p) {
        case 0b00:
            reg = &cpu.registers.BC;
            break;
        case 0b01:
            reg = &cpu.registers.DE;
            break;
        case 0b10:
            reg = &cpu.registers.HL;
            break;
    }
    
    uint32_t result = cpu.registers.HL + *reg;

    cpu.registers.set_flag(F_SUBTRACTION, false);
    cpu.registers.set_flag(F_HALF_CARRY, (cpu.registers.HL & 0x0FFF) + (*reg & 0x0FFF) > 0x0FFF);
    cpu.registers.set_flag(F_CARRY, result > 0xFFFF);

    cpu.registers.HL = (uint16_t)result;

    cpu.registers.PC += 1;
    return 8;
}

int execute_ld16_mem_write(uint8_t opcode){
    uint8_t p = (opcode >> 4) & 0b11;
    uint16_t *reg = &cpu.registers.HL;
    
    switch (p) {
        case 0b00:
            reg = &cpu.registers.BC;
            break;
        case 0b01:
            reg = &cpu.registers.DE;
            break;
    }

    memory.bus_write(*reg, cpu.registers.A);

    if (p == 0b11){
        // HL -
        *reg -= 1;
    } else if (p == 0b10){
        // HL +
        *reg += 1;
    }

    cpu.registers.PC += 1;
    return 8;
}

int execute_ld16_mem_read(uint8_t opcode){
    uint8_t p = (opcode >> 4) & 0b11;
    uint16_t *reg = &cpu.registers.HL;
    
    switch (p) {
        case 0b00:
            reg = &cpu.registers.BC;
            break;
        case 0b01:
            reg = &cpu.registers.DE;
            break;
    }

    cpu.registers.A = memory.bus_read(*reg);

    if (p == 0b11){
        // HL -
        *reg -= 1;
    } else if (p == 0b10){
        // HL +
        *reg += 1;
    }

    cpu.registers.PC += 1;
    return 8;
}

int execute_inc16(uint8_t opcode) {
    uint8_t p = (opcode >> 4) & 0b11;
    uint16_t *reg = get_register16(p);

    *reg += 1;
    cpu.registers.PC += 1;

    return 8;
}

int execute_dec16(uint8_t opcode) {
    uint8_t p = (opcode >> 4) & 0b11;
    uint16_t *reg = get_register16(p);

    *reg -= 1;
    cpu.registers.PC += 1;

    return 8;
}

int execute_z0(uint8_t opcode){
    // Control flow (Anything that doesnt fit in the other categories)
    uint8_t y = (opcode >> 3) & 0b111;

    switch (y) {
        case 0: { // NOP
            cpu.registers.PC++;
            return 4;
        }
        case 1: { // LD [a16], SP
            memory.bus_wwrite(memory.bus_wread(cpu.registers.PC + 1), cpu.registers.SP);
            cpu.registers.PC += 3;
            return 20;
        }
        case 2: { // Stop
            // I am unsure how to implement this
            cpu.registers.PC += 2;
            return 4;
        }
        case 3: { // JR e8 (Relative Jump)
            int8_t offset = (int8_t)memory.bus_read(cpu.registers.PC + 1);
            cpu.registers.PC += (offset + 2);
            return 12;
        }
        case 4: { // JR NZ, e8 (Conditional)
            if (cpu.registers.get_flag(F_ZERO)) {
                cpu.registers.PC += 2;
                return 8;
            }
            int8_t offset = (int8_t)memory.bus_read(cpu.registers.PC + 1);
            cpu.registers.PC += (offset + 2);
            return 12;
        }
        case 5: { // JR Z, e8
            if (cpu.registers.get_flag(F_ZERO) == false) {
                cpu.registers.PC += 2;
                return 8;
            }
            int8_t offset = (int8_t)memory.bus_read(cpu.registers.PC + 1);
            cpu.registers.PC += (offset + 2);
            return 12;
        }
        case 6: { // JR NC, e8
            if (cpu.registers.get_flag(F_CARRY)) {
                cpu.registers.PC += 2;
                return 8;
            }
            int8_t offset = (int8_t)memory.bus_read(cpu.registers.PC + 1);
            cpu.registers.PC += (offset + 2);
            return 12;
        }
        case 7: { // JR C, e8
            if (cpu.registers.get_flag(F_CARRY) == false) {
                cpu.registers.PC += 2;
                return 8;
            }
            int8_t offset = (int8_t)memory.bus_read(cpu.registers.PC + 1);
            cpu.registers.PC += (offset + 2);
            return 12;
        }
    }

    return 0;
}

int execute_z1(uint8_t opcode) {
    bool q = (opcode & (1 << 3)) != 0;

    if (q == 0) return execute_ld16(opcode);

    return execute_add16(opcode);
}

int execute_z2(uint8_t opcode){
    bool q = (opcode & (1 << 3)) != 0;

    if (q == 0) return execute_ld16_mem_write(opcode);

    return execute_ld16_mem_read(opcode);
}

int execute_z3(uint8_t opcode){
    bool q = (opcode & (1 << 3)) != 0;

    if (q == 0) return execute_inc16(opcode);

    return execute_dec16(opcode);
}

int execute_z4(uint8_t opcode) {
    // 8-bit Increment (INC)
    uint8_t y = (opcode >> 3) & 0b111;
    uint8_t val;

    if (y == 6) {
        val = memory.bus_read(cpu.registers.HL);
    } else {
        val = *get_register8(y);
    }

    cpu.registers.set_flag(F_HALF_CARRY, (val & 0x0F) == 0x0F);
    
    val += 1;

    cpu.registers.set_flag(F_ZERO, val == 0);
    cpu.registers.set_flag(F_SUBTRACTION, false);

    if (y == 6) {
        memory.bus_write(cpu.registers.HL, val);
        cpu.registers.PC += 1;
        return 12;
    } else {
        *get_register8(y) = val;
        cpu.registers.PC += 1;
        return 4;
    }
}

int execute_z5(uint8_t opcode) {
    // 8-bit Decrement (DEC)
    uint8_t y = (opcode >> 3) & 0b111;
    uint8_t val;

    if (y == 6) {
        val = memory.bus_read(cpu.registers.HL);
    } else {
        val = *get_register8(y);
    }

    cpu.registers.set_flag(F_HALF_CARRY, (val & 0x0F) == 0x00);
    
    val -= 1;

    cpu.registers.set_flag(F_ZERO, val == 0);
    cpu.registers.set_flag(F_SUBTRACTION, true);

    if (y == 6) {
        memory.bus_write(cpu.registers.HL, val);
        cpu.registers.PC += 1;
        return 12;
    } else {
        *get_register8(y) = val;
        cpu.registers.PC += 1;
        return 4;
    }
}

int execute_z6(uint8_t opcode) {
    // 8-bit Immediate load
    uint8_t y = (opcode >> 3) & 0b111;

    uint8_t immediate = memory.bus_read(cpu.registers.PC + 1);

    if (y == 6) {
        memory.bus_write(cpu.registers.HL, immediate);
        cpu.registers.PC += 2;
        return 12;
    } else {
        *get_register8(y) = immediate;
        cpu.registers.PC += 2;
        return 8;
    }
}

int execute_z7(uint8_t opcode) {
    // accumulator rotates
    uint8_t y = (opcode >> 3) & 0b111;
        uint8_t a = cpu.registers.A;

    switch (y) {
        case 0: { // RLCA
            uint8_t bit7 = (a & 0x80) >> 7;
            cpu.registers.A = (a << 1) | bit7;
            cpu.registers.set_flag(F_ZERO, false);
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, false);
            cpu.registers.set_flag(F_CARRY, bit7 == 1);
            break;
        }
        case 1: { // RRCA
            uint8_t bit0 = a & 0x01;
            cpu.registers.A = (a >> 1) | (bit0 << 7);
            cpu.registers.set_flag(F_ZERO, false);
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, false);
            cpu.registers.set_flag(F_CARRY, bit0 == 1);
            break;
        }
        case 2: { // RLA
            uint8_t bit7 = (a & 0x80) >> 7;
            uint8_t carry = cpu.registers.get_flag(F_CARRY) ? 1 : 0;
            cpu.registers.A = (a << 1) | carry;
            cpu.registers.set_flag(F_ZERO, false);
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, false);
            cpu.registers.set_flag(F_CARRY, bit7 == 1);
            break;
        }
        case 3: { // RRA
            uint8_t bit0 = a & 0x01;
            uint8_t carry = cpu.registers.get_flag(F_CARRY) ? 1 : 0;
            cpu.registers.A = (a >> 1) | (carry << 7);
            cpu.registers.set_flag(F_ZERO, false);
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, false);
            cpu.registers.set_flag(F_CARRY, bit0 == 1);
            break;
        }
        case 4: { // DAA
            uint8_t adjust = 0;
            if (cpu.registers.get_flag(F_HALF_CARRY) || (!cpu.registers.get_flag(F_SUBTRACTION) && (a & 0x0F) > 9)) {
                adjust |= 0x06;
            }
            if (cpu.registers.get_flag(F_CARRY) || (!cpu.registers.get_flag(F_SUBTRACTION) && a > 0x99)) {
                adjust |= 0x60;
                cpu.registers.set_flag(F_CARRY, true);
            }
            a += cpu.registers.get_flag(F_SUBTRACTION) ? -adjust : adjust;
            cpu.registers.A = a;
            cpu.registers.set_flag(F_ZERO, a == 0);
            cpu.registers.set_flag(F_HALF_CARRY, false);
            break;
        }
        case 5: { // CPL
            uint8_t adjust = 0;
            cpu.registers.set_flag(F_SUBTRACTION, true);
            cpu.registers.set_flag(F_HALF_CARRY, true);
            cpu.registers.A = ~cpu.registers.A;
            break;
        }
        case 6: { // SCF
            cpu.registers.set_flag(F_CARRY, 1);
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, false);
            break;
        }
        case 7: { // CCF
            cpu.registers.set_flag(F_CARRY, !cpu.registers.get_flag(F_CARRY));
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, false);
            break;
        }

    }

    cpu.registers.PC += 1;
    return 4;
}


int execute_misc_16bit(uint8_t opcode){
    uint8_t z = opcode & 0b111;
    // In the misc category, the sub category is not y but z

    switch (z) {
        case 0: return execute_z0(opcode);
        case 1: return execute_z1(opcode);
        case 2: return execute_z2(opcode);
        case 3: return execute_z3(opcode);
        case 4: return execute_z4(opcode);
        case 5: return execute_z5(opcode);
        case 6: return execute_z6(opcode);
        case 7: return execute_z7(opcode);
    }

    return 0;
}
