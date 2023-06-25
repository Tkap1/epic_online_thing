

struct s_window
{
	HDC dc;
	HWND handle;
	int width;
	int height;
	s_v2 size;
	s_v2 center;
};



LRESULT window_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);
void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
func WPARAM remap_key_if_necessary(WPARAM vk, LPARAM lparam);
func void apply_event_to_input(s_input* in_input, s_stored_input event);
