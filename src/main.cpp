#include <stdio.h>
#include <thread>
#include <frontend.h>
#include <common.h>

extern void emulator_thread();

int main() {
    frontend::initialize();

    std::thread emu_thread(emulator_thread);

    frontend::main_loop();

    g_state.running.store(false);
    if (emu_thread.joinable()) {
        emu_thread.join();
    }

    frontend::uninitialize();
}