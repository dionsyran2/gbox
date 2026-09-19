#include <frontend.h>
#include <mutex>
#include <common.h>
#include <frontend_input.h>

extern int waiting_for_key;

namespace frontend {
    void render_settings_window(bool *show_settings_window) {
        ImGui::Begin("Button Mapping", show_settings_window);
        ImGui::Text("Click a button to rebind:");

        for (int i = 0; i < 8; i++) {
            const char* keyname;
            const char* gamepadname;
            g_input.get_button_mapping(i, &keyname, &gamepadname);

            ImGui::Text("%s: [%s / %s]", g_input.get_button_name(i), keyname, gamepadname);
            ImGui::SameLine();
            ImGui::PushID(i);
            if (ImGui::Button(waiting_for_key == i ? "Press any key..." : "Rebind")) {
                if (waiting_for_key == i) {
                    waiting_for_key = -1;
                } else {
                    waiting_for_key = i;
                }
            }
            ImGui::PopID();
        }

        ImGui::End();
    }
}