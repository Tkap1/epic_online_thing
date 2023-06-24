//have been shocked, wglGetProc("glBufferData") failed,
//then remembered that integrated graphic has incomplete gl-standards
//for notebooks, choose graphicscard instead of embedded intel-graphics.
//nv and amd look for theese exported flags
_declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;


#define m_gl_funcs \
X(PFNGLBUFFERDATAPROC, glBufferData) \
X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
X(PFNGLGENBUFFERSPROC, glGenBuffers) \
X(PFNGLBINDBUFFERPROC, glBindBuffer) \
X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
X(PFNGLCREATESHADERPROC, glCreateShader) \
X(PFNGLSHADERSOURCEPROC, glShaderSource) \
X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
X(PFNGLATTACHSHADERPROC, glAttachShader) \
X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
X(PFNGLCOMPILESHADERPROC, glCompileShader) \
X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor) \
X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced) \
X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallback) \
X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase) \
X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
X(PFNGLUSEPROGRAMPROC, glUseProgram) \
X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
X(PFNWGLSWAPINTERVALEXTPROC, wglSwapIntervalEXT) \
X(PFNWGLGETSWAPINTERVALEXTPROC, wglGetSwapIntervalEXT) \
X(PFNGLBLENDEQUATIONPROC, glBlendEquation) \
X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
X(PFNGLDELETESHADERPROC, glDeleteShader) \
X(PFNGLUNIFORM1FPROC, glUniform1f)

#define X(type, name) global type name = null;
m_gl_funcs
#undef X

global s_window g_window;
global s_input g_input;
make_list(s_char_event_list, s_char_event, 1024)
s_char_event_list char_event_arr;

func void create_window()
{
	char* class_name = "epic_online_thing_class";
	HINSTANCE instance = GetModuleHandle(null);

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = null;
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = null;

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		dummy start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		WNDCLASSEX window_class = zero;
		window_class.cbSize = sizeof(window_class);
		window_class.style = CS_OWNDC;
		window_class.lpfnWndProc = DefWindowProc;
		window_class.lpszClassName = class_name;
		window_class.hInstance = instance;
		check(RegisterClassEx(&window_class));

		HWND dummy_window = CreateWindowEx(
			0,
			class_name,
			"dummy",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			null,
			null,
			instance,
			null
		);
		assert(dummy_window != INVALID_HANDLE_VALUE);

		HDC dc = GetDC(dummy_window);

		PIXELFORMATDESCRIPTOR pfd = zero;
		pfd.nSize = sizeof(pfd);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
		pfd.cColorBits = 24;
		pfd.cDepthBits = 24;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int format = ChoosePixelFormat(dc, &pfd);
		check(DescribePixelFormat(dc, format, sizeof(pfd), &pfd));
		check(SetPixelFormat(dc, format, &pfd));

		HGLRC glrc = wglCreateContext(dc);
		check(wglMakeCurrent(dc, glrc));

		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)load_gl_func("wglCreateContextAttribsARB");
		wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)load_gl_func("wglChoosePixelFormatARB");

		check(wglMakeCurrent(null, null));
		check(wglDeleteContext(glrc));
		check(ReleaseDC(dummy_window, dc));
		check(DestroyWindow(dummy_window));
		check(UnregisterClass(class_name, instance));

	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		dummy end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		window start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		WNDCLASSEX window_class = zero;
		window_class.cbSize = sizeof(window_class);
		window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		window_class.lpfnWndProc = window_proc;
		window_class.lpszClassName = class_name;
		window_class.hInstance = instance;
		window_class.hCursor = LoadCursor(null, IDC_ARROW);
		check(RegisterClassEx(&window_class));

		DWORD style = (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_MAXIMIZEBOX & ~WS_SIZEBOX;
		RECT rect = zero;
		rect.right = (int)c_base_res.x;
		rect.bottom = (int)c_base_res.y;
		AdjustWindowRect(&rect, style, false);

		g_window.handle = CreateWindowEx(
			0,
			class_name,
			"Epic Online Thing",
			style,
			CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
			null,
			null,
			instance,
			null
		);
		assert(g_window.handle != INVALID_HANDLE_VALUE);

		SetLayeredWindowAttributes(g_window.handle, RGB(0, 0, 0), 0, LWA_COLORKEY);

		g_window.dc = GetDC(g_window.handle);

		int pixel_attribs[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_SWAP_METHOD_ARB, WGL_SWAP_COPY_ARB,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			0
		};

		PIXELFORMATDESCRIPTOR pfd = zero;
		pfd.nSize = sizeof(pfd);
		int format;
		u32 num_formats;
		check(wglChoosePixelFormatARB(g_window.dc, pixel_attribs, null, 1, &format, &num_formats));
		check(DescribePixelFormat(g_window.dc, format, sizeof(pfd), &pfd));
		SetPixelFormat(g_window.dc, format, &pfd);

		int gl_attribs[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
			0
		};
		HGLRC glrc = wglCreateContextAttribsARB(g_window.dc, null, gl_attribs);
		check(wglMakeCurrent(g_window.dc, glrc));

		#define X(type, name) name = (type)load_gl_func(#name);
		m_gl_funcs
		#undef X

		glDebugMessageCallback(gl_debug_callback, null);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		window end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
}

func PROC load_gl_func(char* name)
{
	PROC result = wglGetProcAddress(name);
	if(!result)
	{
		printf("Failed to load %s\n", name);
		assert(false);
	}
	return result;
}

LRESULT window_proc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;

	switch(msg)
	{

		case WM_CLOSE:
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		} break;

		case WM_CHAR:
		{
			s_char_event_list_add(&char_event_arr, (s_char_event){.is_symbol = false, .c = (int)wparam});
		} break;

		case WM_SIZE:
		{
			g_window.width = LOWORD(lparam);
			g_window.height = HIWORD(lparam);
			g_window.size = v2ii(g_window.width, g_window.height);
			g_window.center = v2_mul(g_window.size, 0.5f);
		} break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			int key = (int)remap_key_if_necessary(wparam, lparam);
			b8 is_down = !(b32)((HIWORD(lparam) >> 15) & 1);
			int is_echo = is_down && ((lparam >> 30) & 1);
			if(key < c_max_keys && !is_echo)
			{
				s_stored_input si = zero;
				si.key = key;
				si.is_down = is_down;
				apply_event_to_input(&g_input, si);
			}

			if(is_down)
			{
				s_char_event_list_add(&char_event_arr, (s_char_event){.is_symbol = true, .c = (int)wparam});
			}
		} break;

		default:
		{
			result = DefWindowProc(window, msg, wparam, lparam);
		}
	}

	return result;
}

void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	unreferenced(userParam);
	unreferenced(length);
	unreferenced(id);
	unreferenced(type);
	unreferenced(source);
	if(severity >= GL_DEBUG_SEVERITY_HIGH)
	{
		printf("GL ERROR: %s\n", message);
		assert(false);
	}
}

func b8 is_key_down(int key)
{
	assert(key < c_max_keys);
	return g_input.keys[key].is_down || g_input.keys[key].count >= 2;
}

func b8 is_key_up(int key)
{
	assert(key < c_max_keys);
	return !g_input.keys[key].is_down;
}

func b8 is_key_pressed(int key)
{
	assert(key < c_max_keys);
	return (g_input.keys[key].is_down && g_input.keys[key].count == 1) || g_input.keys[key].count > 1;
}

func b8 is_key_released(int key)
{
	assert(key < c_max_keys);
	return (!g_input.keys[key].is_down && g_input.keys[key].count == 1) || g_input.keys[key].count > 1;
}

// @Note(tkap, 16/05/2023): https://stackoverflow.com/a/15977613
func WPARAM remap_key_if_necessary(WPARAM vk, LPARAM lparam)
{
	WPARAM new_vk = vk;
	UINT scancode = (lparam & 0x00ff0000) >> 16;
	int extended  = (lparam & 0x01000000) != 0;

	switch(vk)
	{
		case VK_SHIFT:
		{
			new_vk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX);
		} break;

		case VK_CONTROL:
		{
			new_vk = extended ? VK_RCONTROL : VK_LCONTROL;
		} break;

		case VK_MENU:
		{
			new_vk = extended ? VK_RMENU : VK_LMENU;
		} break;

		default:
		{
			new_vk = vk;
		} break;
	}

	return new_vk;
}

func void apply_event_to_input(s_input* in_input, s_stored_input event)
{
	in_input->keys[event.key].is_down = event.is_down;
	in_input->keys[event.key].count += 1;
}

func s_char_event get_char_event()
{
	s_char_event event = zero;
	if(char_event_arr.count > 0)
	{
		event = char_event_arr.elements[0];
		char_event_arr.count -= 1;
		int to_move = char_event_arr.count;
		memmove(&char_event_arr.elements[0], &char_event_arr.elements[1], sizeof(char_event_arr.elements[0]) * to_move);
	}
	return event;
}