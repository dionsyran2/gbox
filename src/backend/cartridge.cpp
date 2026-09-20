#include <cartridge.h>
#include <memory/memory.h>
#include <common.h>
#include <cpu/cpu.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

#include <dmg_boot.h>


cartridge_t cartridge = { 0 };

int sram_size[] = {
	0, // No RAM
	0, // Unused
	8 * 1024,
	32 * 1024,
	128 * 1024,
	64 * 1024
};



int cartridge_t::parse_rom(FILE *file) {
	// Get the file size
	fseek(file, 0L, SEEK_END);
	size_t size = ftell(file);
	fseek(file, 0L, SEEK_SET);

	// Allocate the memory
	void *data = malloc(size);

	if (!data) return -ENOMEM;

	// Read the file
	size_t bytes_read = fread(data, 1, size, file);
	

	if (cartridge_data) free(cartridge_data);
	cartridge_data = data;
	current_rom_bank = 1;
	cartridge_header* header = (cartridge_header*)((uintptr_t)data + 0x0100);

	if (ram_buffer)
	{
		free(ram_buffer);
		ram_buffer = nullptr;
	}

	bool is_mbc2 = (header->cartridge_type == 0x05 || header->cartridge_type == 0x06);

	int sram = is_mbc2 ? 512 : sram_size[header->ram_size];

	if (sram) {
		ram_buffer = malloc(sram);
		ram_size = sram; // Store actual byte size, not header code
		g_state.can_save_cartridge.store(true);
	}
	else {
		g_state.can_save_cartridge.store(false);
	}

	mapper_type = header->cartridge_type;

	cpu.reset();
	g_state.paused.store(false, std::memory_order_relaxed);

	return 0;
}


int cartridge_t::load_rom(const char* filename) {
	FILE* file = fopen(filename, "rb");

	if (!file) {
		return errno;
	}

	parse_rom(file);
	fclose(file);

	return 0;
}

bool boot_rom_active = true;

uint8_t cartridge_read(uint16_t address) {
    if (address < 0x100 && boot_rom_active) {
        return dmg_boot_bin[address];
    }

    uint8_t* data = (uint8_t*)cartridge.cartridge_data;

    if (address < 0x4000) return data[address];

    if (address >= 0x4000 && address < 0x8000) {
        uint32_t offset = address - 0x4000;
        uint32_t physical_address = (cartridge.current_rom_bank * 0x4000) + offset;
        return data[physical_address];
    }

    if (address >= 0xA000 && address < 0xC000) {
        if (!cartridge.ram_enabled || !cartridge.ram_buffer) return 0xFF;

        bool is_mbc2 = (cartridge.mapper_type == 0x05 || cartridge.mapper_type == 0x06);
        bool is_mbc3 = (cartridge.mapper_type >= 0x0F && cartridge.mapper_type <= 0x13);

        if (is_mbc2) {
            uint16_t offset = (address - 0xA000) & 0x01FF;
            uint8_t* ram = (uint8_t*)cartridge.ram_buffer;
            return 0xF0 | ram[offset];
        }
        else if (is_mbc3 && cartridge.current_ram_bank >= 0x08 && cartridge.current_ram_bank <= 0x0C) {
            // Read from RTC registers
            cartridge.rtc.update_time();
            switch (cartridge.current_ram_bank) {
                case 0x08: return cartridge.rtc.l_seconds;
                case 0x09: return cartridge.rtc.l_minutes;
                case 0x0A: return cartridge.rtc.l_hours;
                case 0x0B: return cartridge.rtc.l_days_low;
                case 0x0C: return cartridge.rtc.l_flags;
            }
            return 0xFF;
        }
        else {
            uint32_t offset = address - 0xA000;
            uint32_t physical_address = (cartridge.current_ram_bank * 0x2000) + offset;
            uint8_t* ram = (uint8_t*)cartridge.ram_buffer;
            return ram[physical_address];
        }
    }

    return 0xFF;
}

void cartridge_write(uint16_t address, uint8_t value) {
    bool is_mbc2 = (cartridge.mapper_type == 0x05 || cartridge.mapper_type == 0x06);
    bool is_mbc3 = (cartridge.mapper_type >= 0x0F && cartridge.mapper_type <= 0x13);
    bool is_mbc5 = (cartridge.mapper_type >= 0x19 && cartridge.mapper_type <= 0x1E);

    if (address < 0x2000) {
        cartridge.ram_enabled = ((value & 0x0F) == 0x0A);
    }
    else if (address >= 0x2000 && address < 0x3000) {
        if (is_mbc5) {
            cartridge.current_rom_bank = (cartridge.current_rom_bank & 0x100) | value;
        }
        else if (is_mbc3) {
            cartridge.current_rom_bank = value & 0x7F;
            if (cartridge.current_rom_bank == 0) cartridge.current_rom_bank = 1;
        }
        else {
            cartridge.current_rom_bank = value & 0x1F;
            if (cartridge.current_rom_bank == 0) cartridge.current_rom_bank = 1;
        }
    }
    else if (address >= 0x3000 && address < 0x4000) {
        if (is_mbc5) {
            cartridge.current_rom_bank = (cartridge.current_rom_bank & 0xFF) | ((value & 1) << 8);
        }
        else if (is_mbc3) {
            cartridge.current_rom_bank = value & 0x7F;
            if (cartridge.current_rom_bank == 0) cartridge.current_rom_bank = 1;
        }
        else {
            cartridge.current_rom_bank = value & 0x1F;
            if (cartridge.current_rom_bank == 0) cartridge.current_rom_bank = 1;
        }
    }
    else if (address >= 0x4000 && address < 0x6000) {
        // MBC3 allows RAM banks 0-3 OR RTC registers 0x08-0x0C
        if (is_mbc5) {
            cartridge.current_ram_bank = value & 0x0F;
        }
        else if (is_mbc3) {
            if (value <= 0x03 || (value >= 0x08 && value <= 0x0C)) {
                cartridge.current_ram_bank = value;
            }
        }
    }
    else if (address >= 0x6000 && address < 0x8000 && is_mbc3) {
        // Latch sequence: Write 0x00 then 0x01
        if (cartridge.latch_sequence == 0x00 && value == 0x01) {
            cartridge.rtc.update_time();
            cartridge.rtc.latch();
        }
        cartridge.latch_sequence = value;
    }
    else if (address >= 0xA000 && address < 0xC000) {
        if (!cartridge.ram_enabled || !cartridge.ram_buffer) return;

        if (is_mbc2) {
            uint16_t offset = (address - 0xA000) & 0x01FF;
            uint8_t* ram = (uint8_t*)cartridge.ram_buffer;
            ram[offset] = value & 0x0F;
        }
        else if (is_mbc3 && cartridge.current_ram_bank >= 0x08 && cartridge.current_ram_bank <= 0x0C) {
            // Write directly to live RTC registers
            cartridge.rtc.update_time();
            switch (cartridge.current_ram_bank) {
                case 0x08: cartridge.rtc.seconds = value & 0x3F; break;
                case 0x09: cartridge.rtc.minutes = value & 0x3F; break;
                case 0x0A: cartridge.rtc.hours = value & 0x1F; break;
                case 0x0B: cartridge.rtc.days_low = value; break;
                case 0x0C: cartridge.rtc.flags = value; break;
            }
        }
        else {
            uint32_t offset = address - 0xA000;
            uint32_t physical_address = (cartridge.current_ram_bank * 0x2000) + offset;
            uint8_t* ram = (uint8_t*)cartridge.ram_buffer;
            ram[physical_address] = value;
        }
    }
}

void boot_write(uint16_t address, uint8_t value) {
    if (value & 0x01)
        boot_rom_active = false;
}

uint8_t boot_read(uint16_t address) {
    return 0xFF;
}

REGISTER_MEMORY_REGION(boot_read, boot_write, 0xFF50, 0xFF50); // Bootloader
REGISTER_MEMORY_REGION(cartridge_read, cartridge_write, 0x0000, 0x7FFF); // ROM Banks 0 and 1+
REGISTER_MEMORY_REGION(cartridge_read, cartridge_write, 0xA000, 0xBFFF); // Cartridge SRAM

void save_cartridge_sram(const char* path) {
    if (g_state.can_save_cartridge.load() == false) return;

    bool is_mbc3 = (cartridge.mapper_type >= 0x0F && cartridge.mapper_type <= 0x13);
    int actual_bytes = (cartridge.mapper_type == 0x05 || cartridge.mapper_type == 0x06) ? 512 : cartridge.ram_size;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (file.is_open()) {
        // Write standard SRAM data
        if (cartridge.ram_buffer && actual_bytes > 0) {
            file.write(reinterpret_cast<const char*>(cartridge.ram_buffer), actual_bytes);
        }

        // If it's an MBC3 cartridge, append the RTC state to the file!
        if (is_mbc3) {
            cartridge.rtc.update_time();

            file.write(reinterpret_cast<const char*>(&cartridge.rtc), sizeof(cartridge.rtc));
        }

        file.close();
    }
}

void load_cartridge_sram(const char* path) {
	if (g_state.can_save_cartridge.load() == false) return;

    bool has_rtc = (cartridge.mapper_type == 0x0F || cartridge.mapper_type == 0x10);
    int actual_bytes = cartridge.ram_size;

	std::ifstream file(path, std::ios::binary);
	if (file.is_open()) {
        std::streamsize file_size = file.tellg();
		file.read(reinterpret_cast<char*>(cartridge.ram_buffer), cartridge.ram_size);

        if (has_rtc && file_size >= (std::streamsize)(actual_bytes + sizeof(cartridge.rtc))) {
            file.read(reinterpret_cast<char*>(&cartridge.rtc), sizeof(cartridge.rtc));
            cartridge.rtc.update_time();
        }
		file.close();
	}

	cpu.reset();
}