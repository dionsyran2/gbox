#include <frontend.h>
#include <mutex>
#include <common.h>
#include <cpu/registers.h>
#include <cpu/cpu.h>

namespace frontend {
    void render_cpu_debug_window(bool* show_cpu_debug) {
        if (ImGui::Begin("CPU Registers", show_cpu_debug)) {
            registers_t r = cpu.registers;

            ImGui::Text("16-Bit Registers");
            ImGui::Separator();
            ImGui::Text("PC: 0x%04X", r.PC);
            ImGui::Text("SP: 0x%04X", r.SP);
            ImGui::Text("AF: 0x%04X", r.AF);
            ImGui::Text("BC: 0x%04X", r.BC);
            ImGui::Text("DE: 0x%04X", r.DE);
            ImGui::Text("HL: 0x%04X", r.HL);

            ImGui::Spacing();
            ImGui::Text("8-Bit Registers");
            ImGui::Separator();
            if (ImGui::BeginTable("8bit_regs", 2)) {
                ImGui::TableNextColumn(); ImGui::Text("A: 0x%02X", r.A);
                ImGui::TableNextColumn(); ImGui::Text("F: 0x%02X", r.F);
                ImGui::TableNextColumn(); ImGui::Text("B: 0x%02X", r.B);
                ImGui::TableNextColumn(); ImGui::Text("C: 0x%02X", r.C);
                ImGui::TableNextColumn(); ImGui::Text("D: 0x%02X", r.D);
                ImGui::TableNextColumn(); ImGui::Text("E: 0x%02X", r.E);
                ImGui::TableNextColumn(); ImGui::Text("H: 0x%02X", r.H);
                ImGui::TableNextColumn(); ImGui::Text("L: 0x%02X", r.L);
                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::Text("Flags (Z N H C)");
            ImGui::Separator();
            ImGui::Text(" %d %d %d %d",
                (r.F & F_ZERO) ? 1 : 0,
                (r.F & F_SUBTRACTION) ? 1 : 0,
                (r.F & F_HALF_CARRY) ? 1 : 0,
                (r.F & F_CARRY) ? 1 : 0);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("Halt: %d", 0/*cpu.halted*/);
            ImGui::Text("Paused: %d", g_state.paused.load());
        }
        ImGui::End();
    }
}