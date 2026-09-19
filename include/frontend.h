#pragma once
#include <imgui.h>

struct GLFWwindow;

namespace frontend {
    extern GLFWwindow* window;

    bool initialize();
    void main_loop();
    bool uninitialize();

    bool initialize_imgui();
    void imgui_new_frame();

    void render_menu_bar(bool &show_settings_window, bool& show_cpu_debug, bool& show_audio_debug, GLFWwindow* window);
    void render_audio_debug_window(bool* show_audio_debug);
    void render_settings_window(bool *show_settings_window);
    void render_cpu_debug_window(bool* show_cpu_debug);

    void open_rom();
}