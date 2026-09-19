#pragma once

struct GLFWwindow;

enum gamepad_type_t {
	XBOX,
	PlayStation,
	Nintendo,
};

class frontend_input_t {
public:
	void handle_keyboard_input(int scancode, bool state);
	void handle_gamepad_input(int keycode, bool state, gamepad_type_t type);
	int get_button_count();
	const char* get_button_name(int button);
	void get_button_mapping(int button, const char** keyboard, const char** gamepad);
	void set_button_mapping_kb(int button, int key);
	void set_button_mapping_gp(int button, int keycode);

private:
	int waiting_for_key_for_button = -1;
	const char* button_names[8] = { "A", "B", "Select", "Start", "Right", "Left", "Up", "Down" };

	int keyboard_map[8] = {
		88,
		90,
		32,
		257,
		262,
		263,
		265,
		264
	};

	int gamepad_map[8] = { 
		1,
		0,
		6,
		7,
		12,
		14,
		11,
		13,
	};

	gamepad_type_t last_gamepad_type;
};

void input_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void input_poll_gamepad(int jid);

extern frontend_input_t g_input;