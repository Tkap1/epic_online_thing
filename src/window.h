

#define c_max_keys 1024

typedef struct s_window
{
	HDC dc;
	HWND handle;
	int width;
	int height;
	s_v2 size;
	s_v2 center;
} s_window;

typedef struct s_stored_input
{
	b8 is_down;
	int key;
} s_stored_input;

typedef struct s_key
{
	b8 is_down;
	int count;
} s_key;

typedef struct s_input
{
	s_key keys[c_max_keys];
} s_input;


func PROC load_gl_func(char* name);
LRESULT window_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);
void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
func b8 is_key_down(int key);
func b8 is_key_up(int key);
func b8 is_key_pressed(int key);
func b8 is_key_released(int key);
func WPARAM remap_key_if_necessary(WPARAM vk, LPARAM lparam);
func void apply_event_to_input(s_input* in_input, s_stored_input event);