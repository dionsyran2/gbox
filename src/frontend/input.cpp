#include <frontend_input.h>
#include <GLFW/glfw3.h>
#include <cstring>
#include <common.h>
#include <cstdio>


frontend_input_t g_input;

int waiting_for_key = -1;

const char* PS_Controller_Buttons[] = {
    "Cross",      // 0: Bottom
    "Circle",     // 1: Right
    "Square",     // 2: Left
    "Triangle",   // 3: Top
    "L1",         // 4: Left Bumper
    "R1",         // 5: Right Bumper (Was R2!)
    "Share",      // 6: Select/Back (PS4=Share, PS5=Create)
    "Options",    // 7: Start
    "PS Button",  // 8: Guide
    "L3",         // 9: LS Click
    "R3",         // 10: RS Click
    "D-Pad Up",
    "D-Pad Right",
    "D-Pad Down",
    "D-Pad Left",
};

const char* XBOX_Controller_Buttons[] = {
    "A",          // 0: Bottom
    "B",          // 1: Right
    "X",          // 2: Left
    "Y",          // 3: Top
    "LB",         // 4: Left Bumper
    "RB",         // 5: Right Bumper
    "View",       // 6: Select/Back
    "Menu",       // 7: Start
    "Guide",      // 8: Guide
    "LS Click",   // 9: LS Click
    "RS Click",   // 10: RS Click
    "D-Pad Up",
    "D-Pad Right",
    "D-Pad Down",
    "D-Pad Left",
};

const char* SW_Controller_Buttons[] = {
    "B",          // 0: Bottom
    "A",          // 1: Right (Fixed)
    "Y",          // 2: Left  (Fixed)
    "X",          // 3: Top   (Fixed)
    "L",          // 4: Left Bumper
    "R",          // 5: Right Bumper
    "Minus",      // 6: Select/Back
    "Plus",       // 7: Start
    "Home",       // 8: Guide
    "LS Click",   // 9: LS Click
    "RS Click",   // 10: RS Click
    "D-Pad Up",
    "D-Pad Right",
    "D-Pad Down",
    "D-Pad Left",
};
void frontend_input_t::handle_keyboard_input(int keycode, bool state) {
    for (int i = 0; i < 8; i++) {
        if (this->keyboard_map[i] == keycode) {

            uint8_t current_state = g_state.button_states.load(std::memory_order_relaxed);

            if (state == true) {
                current_state |= (1 << i);
            }
            else {
                current_state &= ~(1 << i);
            }

            g_state.button_states.store(current_state, std::memory_order_relaxed);
        }
    }
}

void frontend_input_t::handle_gamepad_input(int keycode, bool state, gamepad_type_t type) {
    this->last_gamepad_type = type;

    for (int i = 0; i < 8; i++) {
        if (this->gamepad_map[i] == keycode) {

            uint8_t current_state = g_state.button_states.load(std::memory_order_relaxed);

            if (state == true) {
                current_state |= (1 << i);
            }
            else {
                current_state &= ~(1 << i);
            }

            g_state.button_states.store(current_state, std::memory_order_relaxed);
            g_state.buttons_updated.store(true, std::memory_order_relaxed);
        }
    }
}

int frontend_input_t::get_button_count() {
	return sizeof(this->button_names) / sizeof(const char*);
}

const char* frontend_input_t::get_button_name(int button) {
	return this->button_names[button];
}

void frontend_input_t::get_button_mapping(int button, const char** keyboard, const char** gamepad) {
	int kb = this->keyboard_map[button];
	int gp = this->gamepad_map[button];

	if (kb == -1) {
		*keyboard = "None";
	}
	else {
        const char* key_name = glfwGetKeyName(kb, 0);

		if (key_name != nullptr) {
			*keyboard = key_name;
		}
		else {
            switch (kb) {
            case GLFW_KEY_SPACE:
                *keyboard = "Space";
                break;
            case GLFW_KEY_ENTER:
                *keyboard = "Enter";
                break;
            case GLFW_KEY_RIGHT:
                *keyboard = "Arrow Right";
                break;
            case GLFW_KEY_LEFT:
                *keyboard = "Arrow Left";
                break;
            case GLFW_KEY_UP:
                *keyboard = "Arrow Up";
                break;
            case GLFW_KEY_DOWN:
                *keyboard = "Arrow Down";
                break;


            default:
                static char fallback_name[32];
                snprintf(fallback_name, sizeof(fallback_name), "Scancode %d", kb);
                *keyboard = fallback_name;
                break;
            }
		}
	}

	if (gp == -1) {
		*gamepad = "None";
	}
	else {
		const char** names = XBOX_Controller_Buttons;
		if (last_gamepad_type == Nintendo) {
			names = SW_Controller_Buttons;
		}
		else if (last_gamepad_type == PlayStation) {
			names = PS_Controller_Buttons;
		}

		*gamepad = XBOX_Controller_Buttons[gp];
	}
}

void frontend_input_t::set_button_mapping_kb(int button, int key) {
	this->keyboard_map[button] = key;
}

void frontend_input_t::set_button_mapping_gp(int button, int keycode) {
	this->gamepad_map[button] = keycode;
}

void input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_REPEAT) return;

    if (waiting_for_key != -1) {
        if (action == GLFW_PRESS) {
            g_input.set_button_mapping_kb(waiting_for_key, key);
            waiting_for_key = -1;
        }
        return;
    }

    g_input.handle_keyboard_input(key, action == GLFW_PRESS);
}

unsigned char gamepad_previous[GLFW_GAMEPAD_BUTTON_LAST + 1] = { 0 };
void input_poll_gamepad(int jid) {
    GLFWgamepadstate state;
    if (glfwJoystickIsGamepad(jid) && glfwGetGamepadState(jid, &state)) {
        gamepad_type_t type = XBOX;
        const char* cname = glfwGetJoystickName(jid);
        if (cname[0] == 'P') {
            // Pro controller
            type = Nintendo;
        }
        else if (cname[0] == 'W' || cname[0] == 'D') {
            // Wireless Controller (PS4) / DualSense Wireless Controller (PS5)
            type = PlayStation;
        }
        // Keep anything else as XBOX

        for (int i = 0; i <= GLFW_GAMEPAD_BUTTON_DPAD_LEFT; i++) {
            if (state.buttons[i] == GLFW_PRESS && gamepad_previous[i] == GLFW_RELEASE) {

                if (waiting_for_key != -1) {
                    g_input.set_button_mapping_gp(waiting_for_key, i);
                    waiting_for_key = -1;
                }
                else {
                    g_input.handle_gamepad_input(i, true, type);
                }

            }
            else if (state.buttons[i] == GLFW_RELEASE && gamepad_previous[i] == GLFW_PRESS) {
                if (waiting_for_key == -1) {
                    g_input.handle_gamepad_input(i, false, type);
                }
            }
        }

        memcpy(gamepad_previous, &state.buttons, sizeof(state.buttons));
    }
}