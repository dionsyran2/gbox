#include <cpu/cpu.h>
#include <memory/memory.h>
#include <common.h>
#include <stdio.h>

int conditional_ret(uint8_t opcode) {
    uint8_t y = (opcode >> 3) & 0b111;

    bool condition_met = false;

    switch (y) {
        case 0: // NZ
            condition_met = !cpu.registers.get_flag(F_ZERO);
            break;
        case 1: // Z
            condition_met = cpu.registers.get_flag(F_ZERO);
            break;
        case 2: // NC
            condition_met = !cpu.registers.get_flag(F_CARRY);
            break;
        case 3: // C
            condition_met = cpu.registers.get_flag(F_CARRY);
            break;
    }

    if (!condition_met) {
        cpu.registers.PC += 1;
        return 8;
    }

    uint16_t ret = pop();
    cpu.registers.PC = ret;
    return 20;
};

int conditional_jump(uint8_t opcode) {
    uint8_t y = (opcode >> 3) & 0b111;

    bool condition_met = false;

    switch (y) {
        case 0: // NZ
            condition_met = !cpu.registers.get_flag(F_ZERO);
            break;
        case 1: // Z
            condition_met = cpu.registers.get_flag(F_ZERO);
            break;
        case 2: // NC
            condition_met = !cpu.registers.get_flag(F_CARRY);
            break;
        case 3: // C
            condition_met = cpu.registers.get_flag(F_CARRY);
            break;
    }

    if (!condition_met) {
        cpu.registers.PC += 3;
        return 12;
    }

    cpu.registers.PC = memory.bus_wread(cpu.registers.PC + 1);
    return 16;
}

int handle_z0(uint8_t opcode) {
    uint8_t y = (opcode >> 3) & 0b111;

    if (y >= 0 && y <= 3) return conditional_ret(opcode);

    if (y == 4){
        uint16_t address = 0xFF00 + memory.bus_read(cpu.registers.PC + 1);
        memory.bus_write(address, cpu.registers.A);
        cpu.registers.PC += 2;
        return 12;
    }

    if (y == 5) { // ADD SP e8
        int8_t e8 = (int8_t)memory.bus_read(cpu.registers.PC + 1);
        cpu.registers.SP = cpu.registers.SP + e8;
        cpu.registers.set_flag(F_ZERO, false);
        cpu.registers.set_flag(F_SUBTRACTION, false);
        uint8_t sp_low = (cpu.registers.SP - e8) & 0xFF;
        uint8_t e8_unsigned = (uint8_t)e8;
        cpu.registers.set_flag(F_HALF_CARRY, (sp_low & 0x0F) + (e8_unsigned & 0x0F) > 0x0F);
        cpu.registers.set_flag(F_CARRY, (sp_low & 0xFF) + (e8_unsigned & 0xFF) > 0xFF);
        cpu.registers.PC += 2;
        return 16;
    }

    if (y == 6) {
        uint16_t address = 0xFF00 + memory.bus_read(cpu.registers.PC + 1);
        cpu.registers.A = memory.bus_read(address);
        cpu.registers.PC += 2;
        return 12;
    }

    if (y == 7) { // LD HL, SP, e8
        int8_t e8 = (int8_t)memory.bus_read(cpu.registers.PC + 1);
        cpu.registers.HL = cpu.registers.SP + e8;
        cpu.registers.set_flag(F_ZERO, false);
        cpu.registers.set_flag(F_SUBTRACTION, false);
        uint8_t sp_low = cpu.registers.SP & 0xFF;
        uint8_t e8_unsigned = (uint8_t)e8;
        cpu.registers.set_flag(F_HALF_CARRY, (sp_low & 0x0F) + (e8_unsigned & 0x0F) > 0x0F);
        cpu.registers.set_flag(F_CARRY, (sp_low & 0xFF) + (e8_unsigned & 0xFF) > 0xFF);
        cpu.registers.PC += 2;
        return 12;
    }

    return 0; // Unreachable
}

int handle_z1(uint8_t opcode) {
    uint8_t y = (opcode >> 3) & 0b111;
    uint8_t p = y >> 1;
    uint8_t q = y & 1;

    if (q == 0) {
        // q == 0: POP operations
        uint16_t val = pop();

        if (p == 0) cpu.registers.BC = val;
        if (p == 1) cpu.registers.DE = val;
        if (p == 2) cpu.registers.HL = val;
        if (p == 3) cpu.registers.AF = val & 0xFFF0;

        cpu.registers.PC += 1;
        return 12;
    } else {
        // q == 1: Returns and pointer operations
        switch (p) { // Unconditional Return
            case 0:
                cpu.registers.PC = pop();
                return 16;
            case 1: // RETI
                cpu.registers.PC = pop();
                cpu.registers.IME = true;
                return 16;
            case 2: // JP HL
                cpu.registers.PC = cpu.registers.HL;
                return 4;
            case 3: // LD SP, HL (Copy HL directly into the Stack Pointer)
                cpu.registers.SP = cpu.registers.HL;
                cpu.registers.PC += 1;
                return 8;
        }
    }

    return 0; // Unreachable
}

int handle_z2(uint8_t opcode) {
    uint8_t y = (opcode >> 3) & 0b111;

    if (y >= 0 && y < 4) {
        return conditional_jump(opcode);
    }

    if (y == 4) { // LDH (C), A
        uint16_t address = 0xFF00 + cpu.registers.C;
        memory.bus_write(address, cpu.registers.A);
        cpu.registers.PC += 1;
        return 8;
    }

    if (y == 5) { // LD (a16), A
        uint16_t address = memory.bus_wread(cpu.registers.PC + 1);
        memory.bus_write(address, cpu.registers.A);
        cpu.registers.PC += 3;
        return 16;
    }

    if (y == 6) { // LDH A, (C) 
        uint16_t address = 0xFF00 + cpu.registers.C;
        cpu.registers.A = memory.bus_read(address);
        cpu.registers.PC += 1;
        return 8;
    }

    if (y == 7) { // LD A, (a16)
        uint16_t address = memory.bus_wread(cpu.registers.PC + 1);
        cpu.registers.A = memory.bus_read(address);
        cpu.registers.PC += 3;
        return 16;
    }
    
    return 0;
}

int handle_prefix(uint8_t opcode);
int handle_z3(uint8_t opcode) {
    uint8_t y = (opcode >> 3) & 0b111;

    switch (y) { 
        case 0: // JP 
            cpu.registers.PC = memory.bus_wread(cpu.registers.PC + 1);
            return 16;
        case 1: // Prefix (TODO)
            return handle_prefix(opcode);
        case 6: // DI
            cpu.registers.IME = false;
            cpu.registers.PC += 1;
            return 4;
        case 7: // EI
            cpu.registers.IME = true;
            cpu.registers.PC += 1;
            return 4;
    }

    printf("Illegal Instruction %.2x\n", opcode);
    g_state.paused.store(true);
    return 0;
}

int handle_z4(uint8_t opcode) {
    uint8_t y = (opcode >> 3) & 0b111;

    if (y > 3) {
        printf("Illegal Instruction %.2x\n", opcode);
        g_state.paused.store(true);
        return 0;
    }

    bool condition_met = false;

    switch (y) {
        case 0: // NZ
            condition_met = !cpu.registers.get_flag(F_ZERO);
            break;
        case 1: // Z
            condition_met = cpu.registers.get_flag(F_ZERO);
            break;
        case 2: // NC
            condition_met = !cpu.registers.get_flag(F_CARRY);
            break;
        case 3: // C
            condition_met = cpu.registers.get_flag(F_CARRY);
            break;
    }

    if (!condition_met) {
        cpu.registers.PC += 3;
        return 12;
    }

    push(cpu.registers.PC + 3);
    cpu.registers.PC = memory.bus_wread(cpu.registers.PC + 1);
    return 24;
}

int handle_z5(uint8_t opcode) {
    uint8_t y = (opcode >> 3) & 0b111;
    uint8_t p = y >> 1;
    uint8_t q = y & 1;

    if (q == 0) {
        uint16_t val = 0;
        
        if (p == 0) val = cpu.registers.BC;
        if (p == 1) val = cpu.registers.DE;
        if (p == 2) val = cpu.registers.HL;
        if (p == 3) val = cpu.registers.AF & 0xFFF0;

        push(val);
        cpu.registers.PC += 1;
        return 16; 
    } else {
        if (p == 0) {
            uint16_t address = memory.bus_wread(cpu.registers.PC + 1);
            
            push(cpu.registers.PC + 3); 
            
            cpu.registers.PC = address;
            return 24; 
        } else {
            printf("Illegal Instruction %.2x\n", opcode);
            g_state.paused.store(true);
            return 0;
        }
    }
}

int handle_z6(uint8_t opcode) {
    uint8_t y = (opcode >> 3) & 0b111;
    
    // Read the immediate value from the byte right after the opcode
    uint8_t value = memory.bus_read(cpu.registers.PC + 1);
    
    switch (y) {
        case 0:{ // Add A, n8
            int result = value + cpu.registers.A;

            uint8_t a = cpu.registers.A;
            cpu.registers.A = (uint8_t)result;

            cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, ((a & 0x0F) + (value & 0x0F)) > 0x0F);
            cpu.registers.set_flag(F_CARRY, result > 0xFF);
            break;
        }

        case 1: { // ADC
            uint8_t a = cpu.registers.A;
            uint8_t carry = cpu.registers.get_flag(F_CARRY) ? 1 : 0;

            int result = a + value + carry;
            int h_result = (a & 0x0F) + (value & 0x0F) + carry;

            cpu.registers.A = (uint8_t)result;

            cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, h_result > 0x0F);
            cpu.registers.set_flag(F_CARRY, result > 0xFF);
            break;
        }

        case 2: { // SUB
            uint8_t a = cpu.registers.A;
            int result = a - value;

            cpu.registers.A = (uint8_t)result;

            cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
            cpu.registers.set_flag(F_SUBTRACTION, true);
            cpu.registers.set_flag(F_HALF_CARRY, (a & 0x0F) < (value & 0x0F));
            cpu.registers.set_flag(F_CARRY, result < 0);
            break;
        }
        
        case 3: { // SBC
            uint8_t a = cpu.registers.A;
            uint8_t carry = cpu.registers.get_flag(F_CARRY) ? 1 : 0;

            int result = a - value - carry;
            int h_result = (a & 0x0F) - (value & 0x0F) - carry;

            cpu.registers.A = (uint8_t)result;

            cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
            cpu.registers.set_flag(F_SUBTRACTION, true);
            cpu.registers.set_flag(F_HALF_CARRY, h_result < 0);
            cpu.registers.set_flag(F_CARRY, result < 0);
            break;
        }

        case 4: { // AND
            cpu.registers.A &= value;
            cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, true); // Hardware quirk: always 1
            cpu.registers.set_flag(F_CARRY, false);
            break;
        }

        case 5: { // XOR
            cpu.registers.A ^= value;
            cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, false);
            cpu.registers.set_flag(F_CARRY, false);
            break;
        }

        case 6: { // OR
            cpu.registers.A |= value;
            cpu.registers.set_flag(F_ZERO, cpu.registers.A == 0);
            cpu.registers.set_flag(F_SUBTRACTION, false);
            cpu.registers.set_flag(F_HALF_CARRY, false);
            cpu.registers.set_flag(F_CARRY, false);
            break;
        }

        case 7: {
            uint8_t a = cpu.registers.A;
            int result = a - value;

            cpu.registers.set_flag(F_ZERO, (uint8_t)result == 0);
            cpu.registers.set_flag(F_SUBTRACTION, true);
            cpu.registers.set_flag(F_HALF_CARRY, (a & 0x0F) < (value & 0x0F));
            cpu.registers.set_flag(F_CARRY, result < 0);
            break;
        }
    }
    
    cpu.registers.PC += 2;
    return 8;
}

int handle_z7(uint8_t opcode) {
    uint8_t y = (opcode >> 3) & 0b111;
    
    // Calculate the hardcoded jump address
    uint16_t address = y * 8; 
    
    // Push PC + 1 because RST is only a 1-byte instruction
    push(cpu.registers.PC + 1);
    
    cpu.registers.PC = address;
    return 16;
}

int execute_branch_stack(uint8_t opcode) {
    // Like x0, here z is the subcategory... kinda
    uint8_t z = opcode & 0b111;

    switch (z) {
        case 0:
            return handle_z0(opcode);
        case 1:
            return handle_z1(opcode);
        case 2:
            return handle_z2(opcode);
        case 3:
            return handle_z3(opcode);
        case 4:
            return handle_z4(opcode);
        case 5:
            return handle_z5(opcode);
        case 6:
            return handle_z6(opcode);
        case 7:
            return handle_z7(opcode);
    }

    return 0; // Unreachable
}