#include <common.h>
#include <thread>
#include <stdio.h>
#include <cpu/cpu.h>
#include <ppu/ppu.h>
#include <timer.h>
#include <cstring>
#include <apu/apu.h>
#include <joypad.h>

void emulator_thread() {
    using clock = std::chrono::steady_clock;
    constexpr int CYCLES_PER_FRAME = 70224;
    constexpr double FRAME_TIME_MS = 1000.0 / 59.7275;

    apu.initialize();

    while (g_state.running.load(std::memory_order_relaxed)) {
        if (g_state.paused.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(
                std::chrono::duration<double, std::milli>(FRAME_TIME_MS)
            );
            continue;
        }

        // Check for buttons update
        if (g_state.buttons_updated.load(std::memory_order_relaxed)) {
            g_state.buttons_updated.store(false, std::memory_order_relaxed);
            joypad.handle_update();
        }

        // Start running the cycles
        auto frame_start = clock::now();

        int cycles_completed = 0;
        while (cycles_completed < CYCLES_PER_FRAME) {
            if (g_state.paused.load(std::memory_order_relaxed)) {
                break;
            }

            int cycles = cpu.step();
            ppu.update(cycles);
            timer.timer_tick(cycles);
            apu.step(cycles);

            cycles_completed += cycles;
        }

        // Copy the ppu output
        
        // Mark as ready
        g_state.frame_ready.store(true, std::memory_order_release);

        // Frame Rate Pacing (Sleep for the remainder of 1/60s
        auto frame_end = clock::now();
        std::chrono::duration<double, std::milli> elapsed = frame_end - frame_start;

        while (elapsed.count() < FRAME_TIME_MS) {
            frame_end = clock::now();
            elapsed = frame_end - frame_start;
            std::this_thread::yield();
        }
        /*if (elapsed.count() < FRAME_TIME_MS) {
            std::this_thread::sleep_for(
                std::chrono::duration<double, std::milli>(FRAME_TIME_MS - elapsed.count())
            );


        }*/
    }

    apu.uninitialize();
}