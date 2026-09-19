#pragma once
#include <chrono>

struct rtc_t {
    uint8_t seconds = 0;
    uint8_t minutes = 0;
    uint8_t hours = 0;
    uint8_t days_low = 0;
    uint8_t flags = 0; // Bit 6 = halt, Bit 7 = carry

    uint64_t last_real_timestamp = 0;

    // Latched copies (what the game actually reads)
    uint8_t l_seconds = 0;
    uint8_t l_minutes = 0;
    uint8_t l_hours = 0;
    uint8_t l_days_low = 0;
    uint8_t l_flags = 0;

    void update_time() {
        if (flags & 0x40) return; // Halt flag is set

        uint64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        if (last_real_timestamp == 0) {
            last_real_timestamp = current_time;
            return;
        }

        uint64_t elapsed = current_time - last_real_timestamp;
        last_real_timestamp = current_time;

        seconds += elapsed;
        minutes += seconds / 60; seconds %= 60;
        hours += minutes / 60; minutes %= 60;

        uint32_t total_days = days_low | ((flags & 1) << 8);
        total_days += hours / 24; hours %= 24;

        if (total_days > 511) {
            flags |= 0x80; // Carry flag
            total_days %= 512;
        }

        days_low = total_days & 0xFF;
        flags = (flags & ~1) | ((total_days >> 8) & 1);
    }

    void latch() {
        l_seconds = seconds;
        l_minutes = minutes;
        l_hours = hours;
        l_days_low = days_low;
        l_flags = flags;
    }
};