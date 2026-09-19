#include <frontend.h>
#include <common.h>
#include <GLFW/glfw3.h>
#include <nfd.h>
#include <stdio.h>
#include <cstdlib>
#include <cartridge.h>

namespace frontend {
    void render_menu_bar(bool &show_settings_window, bool& show_cpu_debug, bool& show_audio_debug, GLFWwindow* window) {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open ROM...")) {
                    open_rom();
                }

                if (ImGui::MenuItem("Save Cartridge RAM As...", nullptr, nullptr, g_state.can_save_cartridge.load())) {
                    bool prev_pause_state = g_state.paused.exchange(true);
                    
                    nfdu8char_t* out_path;
                    nfdu8filteritem_t filters[1] = { {"Save Files", "gbsave" } };

                    nfdresult_t result = NFD_SaveDialog(&out_path, filters, 1, nullptr, nullptr);

                    if (result == NFD_OKAY) {
                        save_cartridge_sram(out_path);

                        free(out_path);
                    }
                    else if (result == NFD_ERROR) {
                        printf("NFD Error: %s\n", NFD_GetError());
                    }

                    g_state.paused.store(prev_pause_state, std::memory_order_relaxed);
                }

                if (ImGui::MenuItem("Load Cartridge RAM...", nullptr, nullptr, g_state.can_save_cartridge.load())) {
                    bool prev_pause_state = g_state.paused.exchange(true);

                    nfdu8char_t* out_path;
                    nfdu8filteritem_t filters[1] = { {"Save Files", "gbsave" } };
                    nfdopendialogu8args_t args = { 0 };
                    args.filterList = filters;
                    args.filterCount = 1;

                    nfdresult_t result = NFD_OpenDialogU8_With(&out_path, &args);

                    if (result == NFD_OKAY) {
                        load_cartridge_sram(out_path);

                        free(out_path);
                    }
                    else if (result == NFD_ERROR) {
                        printf("NFD Error: %s\n", NFD_GetError());
                    }

                    g_state.paused.store(prev_pause_state, std::memory_order_relaxed);
                }

                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Debug")) {
                ImGui::MenuItem("Show CPU Debug", nullptr, &show_cpu_debug);
                ImGui::MenuItem("Show Audio Debug", nullptr, &show_audio_debug);
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("Options")) {
                if (ImGui::MenuItem("Configure Controls", nullptr, &show_settings_window)) {
                    show_settings_window = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }
}