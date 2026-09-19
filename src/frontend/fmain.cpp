#include <frontend.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <nfd.h>
#include <common.h>
#include <frontend_input.h>

SharedState g_state;

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}


namespace frontend {
    GLFWwindow* window = nullptr;

    bool initialize_glfw(){
        glfwSetErrorCallback(glfw_error_callback);

        if (!glfwInit()) {
            fprintf(stderr, "Failed to initialize GLFW!\n");
            return -1;
        }

        window = glfwCreateWindow(640, 480, "GBEmu", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            fprintf(stderr, "Failed to create window instance!\n");
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        glfwSetKeyCallback(window, input_key_callback);
        return true;
    }

    bool initialize(){
        NFD_Init();
        if (!initialize_glfw()) return false;
        if (!initialize_imgui()) return false;

        return true;
    }

    void main_loop(){
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            input_poll_gamepad(GLFW_JOYSTICK_1);
            glClear(GL_COLOR_BUFFER_BIT);
            
            imgui_new_frame();
            glfwSwapBuffers(window);
        }
    }

    bool uninitialize(){
        glfwTerminate();

        return true;
    }
}