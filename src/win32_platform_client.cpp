#define m_client 1

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <gl/GL.h>
#include "external/glcorearb.h"
#include "external/wglext.h"
#include <xinput.h>
#include <xaudio2.h>
#include <stdio.h>
#include <math.h>
#include <winsock2.h>
#include "external/enet/enet.h"

#include "types.h"
#include "utils.h"
#include "memory.h"
#include "epic_math.h"
#include "config.h"
#include "shared_all.h"
#include "platform_shared.h"
#include "shared_client_server.h"
#include "platform_shared_client.h"
#include "win32_platform_client.h"
#include "win32_reload_dll.h"

global s_window g_window;
global s_input g_input;
global s_voice voice_arr[c_max_concurrent_sounds];
global u64 g_cycle_frequency;
global u64 g_start_cycles;
global s_gamepad g_gamepads[XUSER_MAX_COUNT];

global s_sarray<s_char_event, 1024> char_event_arr;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;


#include "memory.cpp"
#include "platform_shared_client.cpp"
#include "enet_shared_client.cpp"

int main(int argc, char** argv)
{
	unreferenced(argc);
	unreferenced(argv);

	if(enet_initialize() != 0)
	{
		error(false);
	}

	create_window((int)c_base_res.x, (int)c_base_res.y);
	if(!init_audio())
	{
		printf("failed to init audio Aware\n");
	}
	init_performance();

	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)load_gl_func("wglSwapIntervalEXT");

	s_platform_funcs platform_funcs = zero;
	platform_funcs.play_sound = play_sound;
	platform_funcs.load_gl_func = (t_load_gl_func)load_gl_func;
	platform_funcs.set_swap_interval = set_swap_interval;

	t_update_game* update_game = null;
	t_parse_packet* parse_packet = null;
	HMODULE dll = null;
	void* game_memory = null;
	s_game_network game_network = zero;
	s_platform_network platform_network = zero;
	s_platform_data platform_data = zero;
	s_lin_arena frame_arena = zero;

	{
		s_lin_arena all = zero;
		all.capacity = 20 * c_mb;

		// @Note(tkap, 26/06/2023): We expect this memory to be zero'd
		all.memory = VirtualAlloc(null, all.capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		game_memory = la_get(&all, c_game_memory);
		game_network.read_arena = make_lin_arena_from_memory(1 * c_mb, la_get(&all, 1 * c_mb));
		game_network.write_arena = make_lin_arena_from_memory(1 * c_mb, la_get(&all, 1 * c_mb));
		frame_arena = make_lin_arena_from_memory(5 * c_mb, la_get(&all, 5 * c_mb));
		platform_data.frame_arena = &frame_arena;

	}

	b8 running = true;
	f64 time_passed = 0;
	while(running)
	{
		f64 start_of_frame_seconds = get_seconds();

		MSG msg = zero;
		while(PeekMessage(&msg, null, 0, 0, PM_REMOVE) > 0)
		{
			if(msg.message == WM_QUIT)
			{
				running = false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		do_gamepad_shit();

		platform_data.input = &g_input;
		platform_data.quit_after_this_frame = !running;
		platform_data.window_width = g_window.width;
		platform_data.window_height = g_window.height;
		platform_data.time_passed = time_passed;
		platform_data.char_event_arr = &char_event_arr;

		if(need_to_reload_dll("build/client.dll"))
		{
			if(dll) { unload_dll(dll); }

			for(int i = 0; i < 100; i++)
			{
				if(CopyFile("build/client.dll", "client.dll", false)) { break; }
				Sleep(10);
			}
			dll = load_dll("client.dll");
			update_game = (t_update_game*)GetProcAddress(dll, "update_game");
			assert(update_game);
			parse_packet = (t_parse_packet*)GetProcAddress(dll, "parse_packet");
			assert(parse_packet);
			printf("Reloaded DLL!\n");
			platform_data.recompiled = true;
			update_game(platform_data, platform_funcs, &game_network, game_memory, true);
		}


		POINT p;
		GetCursorPos(&p);
		ScreenToClient(g_window.handle, &p);
		platform_data.mouse.x = (float)p.x;
		platform_data.mouse.y = (float)p.y;

		if(game_network.connect_to_server)
		{
			game_network.connect_to_server = false;
			connect_to_server(&platform_network, &game_network);
		}
		if(game_network.connected)
		{
			enet_loop(platform_network.client, 0, &game_network, parse_packet);
		}

		update_game(platform_data, platform_funcs, &game_network, game_memory, false);
		platform_data.recompiled = false;

		if(game_network.connected && game_network.disconnect)
		{
			enet_peer_disconnect(platform_network.server, 0);
			enet_loop(platform_network.client, 1000, &game_network, parse_packet);
		}

		foreach_raw(packet_i, packet, game_network.out_packets)
		{
			ENetPacket* enet_packet = enet_packet_create(packet.data, packet.size, packet.flag);
			enet_peer_send(platform_network.server, 0, enet_packet);
		}
		game_network.out_packets.count = 0;
		game_network.read_arena.used = 0;
		game_network.write_arena.used = 0;

		SwapBuffers(g_window.dc);

		time_passed = get_seconds() - start_of_frame_seconds;
	}

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
			char_event_arr.add({.is_symbol = false, .c = (int)wparam});
		} break;

		case WM_SIZE:
		{
			g_window.width = LOWORD(lparam);
			g_window.height = HIWORD(lparam);
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
				char_event_arr.add({.is_symbol = true, .c = (int)wparam});
			}
		} break;

		default:
		{
			result = DefWindowProc(window, msg, wparam, lparam);
		}
	}

	return result;
}

func void create_window(int width, int height)
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
		rect.right = width;
		rect.bottom = height;
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

func b8 init_audio()
{
	HRESULT hr = CoInitializeEx(null, COINIT_MULTITHREADED);
	if(FAILED(hr)) { return false; }

	IXAudio2* xaudio2 = null;
	hr = XAudio2Create(&xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if(FAILED(hr)) { return false; }

	IXAudio2MasteringVoice* master_voice = null;
	hr = xaudio2->CreateMasteringVoice(&master_voice);
	if(FAILED(hr)) { return false; }

	WAVEFORMATEX wave = zero;
	wave.wFormatTag = WAVE_FORMAT_PCM;
	wave.nChannels = c_num_channels;
	wave.nSamplesPerSec = c_sample_rate;
	wave.wBitsPerSample = 16;
	wave.nBlockAlign = (c_num_channels * wave.wBitsPerSample) / 8;
	wave.nAvgBytesPerSec = c_sample_rate * wave.nBlockAlign;

	for(int voice_i = 0; voice_i < c_max_concurrent_sounds; voice_i++)
	{
		s_voice* voice = &voice_arr[voice_i];
		hr = xaudio2->CreateSourceVoice(&voice->voice, &wave, 0, XAUDIO2_DEFAULT_FREQ_RATIO, voice, null, null);
		voice->voice->SetVolume(0.25f);
		if(FAILED(hr)) { return false; }
	}

	return true;

}

func b8 play_sound(s_sound sound)
{
	assert(sound.sample_count > 0);
	assert(sound.samples);

	XAUDIO2_BUFFER buffer = zero;
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.AudioBytes = sound.sample_count * c_num_channels * sizeof(s16);
	buffer.pAudioData = (BYTE*)sound.samples;

	s_voice* curr_voice = null;
	for(int voice_i = 0; voice_i < c_max_concurrent_sounds; voice_i++)
	{
		s_voice* voice = &voice_arr[voice_i];
		if(!voice->playing)
		{
			if(thread_safe_set_bool_to_true(&voice->playing))
			{
				curr_voice = voice;
				break;
			}
		}
	}

	if(curr_voice == null) { return false; }

	HRESULT hr = curr_voice->voice->SubmitSourceBuffer(&buffer);
	if(FAILED(hr)) { return false; }

	curr_voice->voice->Start();
	// curr_voice->sound = sound;

	return true;
}

func b8 thread_safe_set_bool_to_true(volatile int* var)
{
	int belief = *var;
	if(!belief)
	{
		int reality = InterlockedCompareExchange((LONG*)var, true, false);
		if(reality == belief) { return true; }
	}
	return false;
}

func void init_performance()
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&g_cycle_frequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&g_start_cycles);
}

func f64 get_seconds()
{
	u64 now;
	QueryPerformanceCounter((LARGE_INTEGER*)&now);
	return (now - g_start_cycles) / (f64)g_cycle_frequency;
}

func void do_gamepad_shit(void)
{
	int buttons[] = {
		XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_START,
		XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB, XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER,
		XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y
	};

	for(int gamepad_i = 0; gamepad_i < XUSER_MAX_COUNT; gamepad_i++)
	{
		s_gamepad* gamepad = &g_gamepads[gamepad_i];
		XINPUT_STATE xinput_state = zero;
		DWORD dwResult = XInputGetState(gamepad_i, &xinput_state);

		if(dwResult == ERROR_SUCCESS)
		{
			gamepad->left_thumb_x = xinput_state.Gamepad.sThumbLX;
			for(int button_i = 0; button_i < array_count(buttons); button_i++)
			{
				if(xinput_state.Gamepad.wButtons & buttons[button_i])
				{
					gamepad->buttons |= buttons[button_i];
				}
			}
		}
		else
		{
		}
	}

	struct s_button_to_key
	{
		int button;
		int key;
	};

	s_button_to_key button_to_key_arr[] = {
		// {XINPUT_GAMEPAD_DPAD_UP, key_space},
		{XINPUT_GAMEPAD_DPAD_DOWN, c_key_down},
		{XINPUT_GAMEPAD_DPAD_LEFT, c_key_left},
		{XINPUT_GAMEPAD_DPAD_RIGHT, c_key_right},
		{XINPUT_GAMEPAD_A, c_key_space},
		{XINPUT_GAMEPAD_B, c_key_down},
		{XINPUT_GAMEPAD_X, c_key_down},
		{XINPUT_GAMEPAD_Y, c_key_down},
	};

	for(int gamepad_i = 0; gamepad_i < XUSER_MAX_COUNT; gamepad_i++)
	{
		s_gamepad* gamepad = &g_gamepads[gamepad_i];

		for(int map_i = 0; map_i < array_count(button_to_key_arr); map_i++)
		{
			s_button_to_key map = button_to_key_arr[map_i];
			b8 is_down = (gamepad->buttons & map.button) != 0;
			b8 was_down = (gamepad->previous_buttons & map.button) != 0;

			if(is_down && !was_down)
			{
				s_stored_input event = zero;
				event.is_down = true;
				event.key = map.key;
				apply_event_to_input(&g_input, event);
			}
			else if(!is_down && was_down)
			{
				s_stored_input event = zero;
				event.is_down = false;
				event.key = map.key;
				apply_event_to_input(&g_input, event);
			}
		}

		b8 right_now = gamepad->left_thumb_x > 2000;
		b8 right_before = gamepad->previous_left_thumb_x > 2000;
		b8 left_now = gamepad->left_thumb_x < -2000;
		b8 left_before = gamepad->previous_left_thumb_x < -2000;
		if(right_now && !right_before)
		{
			s_stored_input event = zero;
			event.is_down = true;
			event.key = c_key_right;
			apply_event_to_input(&g_input, event);
		}
		else if(!right_now && right_before)
		{
			s_stored_input event = zero;
			event.is_down = false;
			event.key = c_key_right;
			apply_event_to_input(&g_input, event);
		}
		if(left_now && !left_before)
		{
			s_stored_input event = zero;
			event.is_down = true;
			event.key = c_key_left;
			apply_event_to_input(&g_input, event);
		}
		else if(!left_now && left_before)
		{
			s_stored_input event = zero;
			event.is_down = false;
			event.key = c_key_left;
			apply_event_to_input(&g_input, event);
		}

		gamepad->previous_buttons = gamepad->buttons;
		gamepad->buttons = 0;
		gamepad->previous_left_thumb_x = gamepad->left_thumb_x;
		gamepad->left_thumb_x = 0;

	}
}

func void set_swap_interval(int interval)
{
	if(wglSwapIntervalEXT)
	{
		wglSwapIntervalEXT(interval);
	}
}
