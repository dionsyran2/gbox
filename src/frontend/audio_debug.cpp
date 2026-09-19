#include <frontend.h>
#include <mutex>
#include <common.h>

namespace frontend {
    void render_audio_debug_window(bool* show_audio_debug) {
        if (ImGui::Begin("Audio Debug", show_audio_debug)) {
            std::lock_guard<std::mutex> lock(g_state.audio_mtx);

            // Calculate the width of a label-less checkbox to perfectly align the Mixer later
            float checkbox_offset = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemSpacing.x;

            // Channel 1
            bool ch1 = g_state.apu_ch1_enable.load();
            if (ImGui::Checkbox("##en1", &ch1)) g_state.apu_ch1_enable.store(ch1);
            ImGui::SameLine();
            ImGui::PlotLines("CH1", g_state.ch1_wave_history, IM_ARRAYSIZE(g_state.ch1_wave_history), g_state.ch1_history_index, "Waveform", -1.0f, 1.0f, ImVec2(0, 80));

            // Channel 2
            bool ch2 = g_state.apu_ch2_enable.load();
            if (ImGui::Checkbox("##en2", &ch2)) g_state.apu_ch2_enable.store(ch2);
            ImGui::SameLine();
            ImGui::PlotLines("CH2", g_state.ch2_wave_history, IM_ARRAYSIZE(g_state.ch2_wave_history), g_state.ch2_history_index, "Waveform", -1.0f, 1.0f, ImVec2(0, 80));

            // Channel 3
            bool ch3 = g_state.apu_ch3_enable.load();
            if (ImGui::Checkbox("##en3", &ch3)) g_state.apu_ch3_enable.store(ch3);
            ImGui::SameLine();
            ImGui::PlotLines("CH3", g_state.ch3_wave_history, IM_ARRAYSIZE(g_state.ch3_wave_history), g_state.ch3_history_index, "Waveform", -1.0f, 1.0f, ImVec2(0, 80));

            // Channel 4
            bool ch4 = g_state.apu_ch4_enable.load();
            if (ImGui::Checkbox("##en4", &ch4)) g_state.apu_ch4_enable.store(ch4);
            ImGui::SameLine();
            ImGui::PlotLines("CH4", g_state.ch4_wave_history, IM_ARRAYSIZE(g_state.ch4_wave_history), g_state.ch4_history_index, "Waveform", -1.0f, 1.0f, ImVec2(0, 80));

            // Shift the Mixer over so it aligns flush with the plots above it
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + checkbox_offset);
            ImGui::PlotLines("Mixer", g_state.mixer_wave_history, IM_ARRAYSIZE(g_state.mixer_wave_history), g_state.mixer_history_index, "Waveform", -1.0f, 1.0f, ImVec2(0, 80));
        }
        ImGui::End();
    }
}