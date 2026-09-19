#include <cpu/cpu.h>
#include <memory/memory.h>

uint8_t *get_register8(uint8_t y); // Defined in the misc.cpp file

int execute_8bit_load(uint8_t opcode){
    if (opcode == 0x76) {
        // The only exception... halt
        // This would be ld [HL], [HL]
        // But because this would not be possible /
        // Wouldn't make any sense it was repurposed as HALT
        cpu.halted = true;
        cpu.registers.PC += 1;
        return 4;
    }

    uint8_t destination = (opcode >> 3) & 0b111; // y
    uint8_t source = opcode & 0b111; // z

    uint8_t val;
    int cycles = 4; // Register -> Register takes 4 cycles

    if (source == 6) { // LD destination, [HL]
        val = memory.bus_read(cpu.registers.HL);
        cycles += 4;
    } else {
        val = *get_register8(source);
    }

    if (destination == 6) { // LD [HL], src
        memory.bus_write(cpu.registers.HL, val);
        cycles += 4;    
    } else {
        *get_register8(destination) = val;
    }
    
    cpu.registers.PC += 1;
    return cycles;
}