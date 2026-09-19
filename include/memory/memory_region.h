#pragma once
#include <stdint.h>

typedef void (*mem_write_cb) (uint16_t address, uint8_t value);
typedef uint8_t (*mem_read_cb) (uint16_t address);

mem_read_cb* get_read_map();
mem_write_cb* get_write_map();

struct mem_reg_int {
    mem_reg_int(mem_read_cb read, mem_write_cb write, uint16_t region_start, uint16_t region_end);
};

#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)
#define REGISTER_MEMORY_REGION(read_cb, write_cb, region_start, region_end) \
    static mem_reg_int MACRO_CONCAT(_mem_reg_, __LINE__)(read_cb, write_cb, region_start, region_end)