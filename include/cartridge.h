#pragma once
#include <stdint.h>
#include <stdio.h>
#include <rtc.h>

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

PACK(
struct cartridge_header {
	uint8_t entry[4];
	uint8_t nintendo_logo[48];
	char title[15];
	uint8_t cgb_flag;
	uint8_t new_licensee_code[2];
	uint8_t sgb_flag;
	uint8_t cartridge_type;
	uint8_t rom_size;
	uint8_t ram_size;
	uint8_t destination_code;
	uint8_t old_license_code;
	uint8_t mask_row_version_number;
	uint8_t header_checksum;
	uint8_t global_checksum_hi;
	uint8_t global_checksum_lo;
}
);

struct cartridge_t {
	rtc_t rtc;
	
    void *cartridge_data;
    size_t cartridge_size;

    int current_rom_bank;

    void *ram_buffer;
    int ram_size;
	int current_ram_bank;
    bool ram_enabled;
    
    uint8_t mapper_type;

	/* RTC */
	int latch_sequence;

    int load_rom(const char *filename);
    int parse_rom(FILE *file);
};

extern cartridge_t cartridge;

void load_cartridge_sram(const char* path);
void save_cartridge_sram(const char* path);