#include <frontend.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <common.h>
#include <cmath>

namespace frontend {
    GLuint screen_texture;
    
    bool show_settings_window = false;
    bool show_cpu_debug = false;
    bool show_audio_debug = false;
    
    bool initialize_imgui() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; 
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");

        glGenTextures(1, &screen_texture);
        glBindTexture(GL_TEXTURE_2D, screen_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GBScreenWidth, GBScreenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        return true;
    }

    void render_gameboy_screen(GLFWwindow* window, GLuint screen_texture) {
        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImVec2 work_pos = viewport->WorkPos;
        ImVec2 work_size = viewport->WorkSize;

        float scale = std::fmin(
            work_size.x / GBScreenWidth,
            work_size.y / GBScreenHeight
        );

        float scaled_w = GBScreenWidth * scale;
        float scaled_h = GBScreenHeight * scale;

        float offset_x = work_pos.x + (work_size.x - scaled_w) / 2.0f;
        float offset_y = work_pos.y + (work_size.y - scaled_h) / 2.0f;

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList(viewport);

        draw_list->AddCallback(
            ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest
        );

        draw_list->AddImage(
            (ImTextureID)(intptr_t)screen_texture,
            ImVec2(offset_x, offset_y),
            ImVec2(offset_x + scaled_w, offset_y + scaled_h)
        );

        draw_list->AddCallback(
            ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear
        );
    }

    void imgui_new_frame() {
        if (g_state.frame_ready.exchange(false, std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lock(g_state.screen_mtx);
            glBindTexture(GL_TEXTURE_2D, screen_texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GBScreenWidth, GBScreenHeight, GL_RGBA, GL_UNSIGNED_BYTE, g_state.screen);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        render_menu_bar(show_settings_window, show_cpu_debug, show_audio_debug, window);

        render_gameboy_screen(window, screen_texture);
        
        if (show_audio_debug) {
            render_audio_debug_window(&show_audio_debug);
        }

        if (show_settings_window) {
            render_settings_window(&show_settings_window);
        }

        if (show_cpu_debug) {
            render_cpu_debug_window(&show_cpu_debug);
        }

        ImGui::Render();
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO(); 
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }
}