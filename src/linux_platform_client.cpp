#define m_client 1

#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glext.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <dlfcn.h>
#include "external/enet/enet.h"
#include "memory.h"
#include "types.h"
#include "utils.h"
#include "epic_math.h"
#include "shared_client_server.h"
#include "config.h"
#include "shared_all.h"
#include "platform_shared_client.h"
#include "platform_shared.h"

#include "memory.cpp"
#include "platform_shared_client.cpp"

#define EPIC_DLLEXPORT

struct s_window
{
	Display *display;
	int screen;
	Window root;
	Window window;
	int width;
	int height;
	GLXContext gl_context;
	Cursor invisible_cursor;
	XIC input_context;
};


struct s_platform_network
{
	ENetHost* client;
	ENetPeer* server;
};

#include "enet_shared_client.cpp"

global s_window g_window;
global s_input g_input;
global s_sarray<s_char_event, 1024> char_event_arr;

enum {
	EventMask =
		ExposureMask
		//|PointerMotionMask
		|KeyPressMask
		|KeyReleaseMask
		|StructureNotifyMask,
};

func void create_window(int InitialWindowWidth, int InitialWindowHeight)
{
	static const GLint visual_attrs[] = {
		GLX_X_RENDERABLE    , True,
		GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
		GLX_RENDER_TYPE     , GLX_RGBA_BIT,
		GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
		GLX_RED_SIZE        , 8,
		GLX_GREEN_SIZE      , 8,
		GLX_BLUE_SIZE       , 8,
		GLX_ALPHA_SIZE      , 8,
		GLX_DEPTH_SIZE      , 24,
		GLX_STENCIL_SIZE    , 8,
		GLX_DOUBLEBUFFER    , True,
		None
	};
	static const int gl_context_attrs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
		GLX_CONTEXT_MINOR_VERSION_ARB, 3,
		GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
		None,
	};
	g_window.display = XOpenDisplay(NULL);
	if(!g_window.display){
		fprintf(stderr, "\ncan't connect to X server\n");
		return;
	}
	g_window.screen = DefaultScreen(g_window.display);
	{
		// making a hidden cursor, since xlib doesn't have a builtin way to
		// hide the mouse pointer. credits to xterm by Thomas Dickey.
		// alternatively, there's also an xfixes extension that has easy to use
		// XFixesHideCursor and XFixesShowCursor functions but that would
		// mean linking to yet another library just to use 2 functions,
		// and so I prefer this approach.
		XFontStruct *fn = XLoadQueryFont(g_window.display, "nil2");
		if (fn) {
			XColor dummy;
			g_window.invisible_cursor = XCreateGlyphCursor(g_window.display, fn->fid, fn->fid, 'X', ' ', &dummy, &dummy);
			XFreeFont(g_window.display, fn);
		}
	}

	GLXFBConfig best_fbc;
	XVisualInfo *best_vi;
	{
		int fbcount;
		GLXFBConfig *fbc =  glXChooseFBConfig(g_window.display, g_window.screen, visual_attrs, &fbcount);
		int best_fbc_i = -1;
		int best_num_samp = -1;
		for (int i = 0; i < fbcount; ++i) {
			XVisualInfo *vi = glXGetVisualFromFBConfig(g_window.display, fbc[i]);
			if (vi) {
				int samp_buf, samples;
				glXGetFBConfigAttrib(g_window.display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
				glXGetFBConfigAttrib(g_window.display, fbc[i], GLX_SAMPLES, &samples);
				if (best_fbc_i<0 || (samp_buf && samples > best_num_samp)) {
					best_fbc_i = i;
					best_num_samp = samples;
				}
				XFree(vi);
			}
		}
		best_fbc = fbc[best_fbc_i];
		best_vi = glXGetVisualFromFBConfig(g_window.display, best_fbc);
		g_window.root = RootWindow(g_window.display, best_vi->screen);
		XFree(fbc);
	}

	Colormap cmap = XCreateColormap(g_window.display, g_window.root, best_vi->visual, AllocNone);
	ulong swa_valuemask = CWBorderPixel|CWColormap|CWEventMask;
	XSetWindowAttributes swa;
	swa.background_pixmap = None,
	swa.border_pixel = 0,
	swa.event_mask = EventMask,
	swa.colormap = cmap,
	// display, parent, x,y,w,h,border,depth,window class, visual, attrs
	g_window.window = XCreateWindow(g_window.display, g_window.root, 0, 0, InitialWindowWidth, InitialWindowHeight, 0, best_vi->depth, InputOutput, best_vi->visual, swa_valuemask, &swa);
	g_window.width = InitialWindowWidth;
	g_window.height = InitialWindowHeight;
	XFree(best_vi);
	if (!g_window.window) {
		fprintf(stderr, "failed to open window\n");
		return;
	}

	XSetLocaleModifiers("");
	XIM input_method = XOpenIM(g_window.display, 0, 0, 0);
	if(!input_method){
		// fallback to internal input method
		XSetLocaleModifiers("@im=none");
		input_method = XOpenIM(g_window.display, 0, 0, 0);
	}
	XIC input_context = XCreateIC(
		input_method,
		XNInputStyle,   XIMPreeditNothing | XIMStatusNothing,
		XNClientWindow, g_window.window,
		XNFocusWindow,  g_window.window,
		NULL);
	XSetICFocus(input_context);
	g_window.input_context = input_context;

	XMapWindow(g_window.display, g_window.window);
	XStoreName(g_window.display, g_window.window, "Epic Online Thing");
	PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARBProc = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
	// display, fbc, share list, direct rendering flag, context attrs
	g_window.gl_context = glXCreateContextAttribsARB(g_window.display, best_fbc, NULL, GL_TRUE, gl_context_attrs);
	glXMakeCurrent(g_window.display, g_window.window, g_window.gl_context);
	glViewport(0,0,InitialWindowWidth,InitialWindowHeight);
}

func b8 handle_input_events(void)
{
	b8 running = true;
	int newwidth = 0;
	int newheight = 0;
	XEvent ev;
	char text[32];
	XKeyEvent *xkey;
	while(XCheckWindowEvent(g_window.display, g_window.window, EventMask, &ev)) {
		switch(ev.type) {
		case Expose:
			break;
		case KeyPress:
		case KeyRelease:
			{
			xkey = &ev.xkey;
			s_char_event char_event = zero;
			KeySym sym = XLookupKeysym(xkey, 0);
			switch(sym) {
			case XK_KP_Add: sym = c_key_add; break;
			case XK_KP_Subtract: sym = c_key_subtract; break;
			case XK_w: sym = c_key_w; break;
			case XK_a: sym = c_key_a; break;
			case XK_s: sym = c_key_s; break;
			case XK_d: sym = c_key_d; break;
			case XK_Home: sym = c_key_home; break;
			case XK_Left: sym = c_key_left; break;
			case XK_Up: sym = c_key_up; break;
			case XK_Right: sym = c_key_right; break;
			case XK_Down: sym = c_key_down; break;
			case XK_End: sym = c_key_end; break;
			case XK_Delete: sym = c_key_delete; break;
			case XK_Return: sym = c_key_enter; break;
			case XK_Tab: sym = c_key_tab; break;
			case XK_BackSpace: sym = c_key_backspace; break;
			case XK_Escape: sym = c_key_escape; running = 0; break;
			}
			b8 is_down = ev.type == KeyPress;
			if (sym < c_max_keys) {
				s_stored_input si;
				si.key = sym,
				si.is_down = is_down,
				apply_event_to_input(&g_input, si);
			}
			Status status;
			b8 is_ctrl_down = (xkey->state & ControlMask) != 0;
			if (is_down && !is_ctrl_down) {
				int len = Xutf8LookupString(g_window.input_context, xkey, text, sizeof(text)-1, &sym, &status);
				text[len] = 0;
				b8 add_text_key = 1;
				switch(status) {
					case XLookupChars: char_event.c = text[0]; break;
					case XLookupBoth: char_event.c = text[0]; break;
					default:
						add_text_key = 0;
				}
				char_event.is_symbol = (sym < 32) || (sym > 126);
				if (add_text_key) {
					char_event_arr.add(char_event);
				}
			}
			} break;
		case ConfigureNotify:
			if (ev.xconfigure.width
				&& ev.xconfigure.height
				&& (ev.xconfigure.width != g_window.width
					|| ev.xconfigure.height != g_window.height))
			{
				newwidth = ev.xconfigure.width;
				newheight = ev.xconfigure.height;
			}
			break;
		}
	}
	if(newwidth > 0 && newheight > 0) {
		glViewport(0,0,newwidth,newheight);
		g_window.width = newwidth;
		g_window.height = newheight;
	}
	return running;
}


// because of our m_gl_func macro setup, the name passed in here is not the
// actual function name, but the name of the function pointer variable. so we
// first extract the function name from it here and then load that function.
func void (*load_gl_func(const char *name))(void)
{
	char buf[1024];
	const char *proc = "Proc";
	int m = 0;
	int found = 0;
	for (int i = 0; i < 1024; ++i) {
		buf[i] = name[i];
		if (name[i] == proc[m]) {
			m++;
			if (m == 5) {
				buf[i-m+1] = 0;
				found = 1;
				break;
			}
		} else {
			m = 0;
		}
	}
	assert(found);
	void (*res)() = glXGetProcAddress((const GLubyte*)buf);
	return res;
}

void set_swap_interval(int interval)
{
	glXSwapIntervalEXT(g_window.display, g_window.window, interval);
}


func u64 get_ticks(void)
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	u64 t = ts.tv_sec * 1000000000L + ts.tv_nsec;
	return t;
}

func f64 get_seconds(u64 start_cycles)
{
	u64 now = get_ticks();
	f64 seconds = (f64)(now - start_cycles) / (f64)1000000000.0;
	return seconds;
}


int main(void)
{
	create_window((int)c_base_res.x, (int)c_base_res.y);
	u64 start_cycles = get_ticks();

	s_lin_arena all = zero;
	all.capacity = 10 * c_mb;
	all.memory = calloc(all.capacity, 1);

	void *game_memory = la_get(&all, 1 * c_mb);
	s_game_network game_network = zero;
	s_platform_network platform_network = zero;
	s_platform_data platform_data = zero;
	game_network.read_arena = make_lin_arena_from_memory(1 * c_mb, la_get(&all, 1*c_mb));
	game_network.write_arena = make_lin_arena_from_memory(1 * c_mb, la_get(&all, 1*c_mb));
	platform_data.frame_arena = make_lin_arena_from_memory(5 * c_mb, la_get(&all, 5*c_mb));
	glXSwapIntervalEXTProc = (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddress((const GLubyte*)"glXSwapIntervalEXT");

	s_platform_funcs platform_funcs = zero;
	platform_funcs.load_gl_func = (t_load_gl_func)load_gl_func;
	platform_funcs.set_swap_interval = set_swap_interval;

	int dl_flags = RTLD_NOW; // resolve function symbols immediately upon loading
	void *client_so = dlopen("./client.so", dl_flags);
	assert(client_so);
	t_update_game *update_game_proc = (t_update_game*)dlsym(client_so, "update_game");
	assert(update_game_proc);
	t_parse_packet *parse_packet_proc = (t_parse_packet*)dlsym(client_so, "parse_packet");
	assert(parse_packet_proc);

	b8 running = true;
	f64 time_passed = 0;
	platform_data.recompiled = true;
	while(running)
	{
		f64 start_of_frame_seconds = get_seconds(start_cycles);

		running = handle_input_events();

		//do_gamepad_shit();

		platform_data.input = &g_input;
		platform_data.quit_after_this_frame = !running;
		platform_data.window_width = g_window.width;
		platform_data.window_height = g_window.height;
		platform_data.time_passed = time_passed;
		platform_data.char_event_arr = &char_event_arr;
		{
			// TODO: hot reload client so and set platform_data.recompiled to true again
		}

		if(game_network.connect_to_server)
		{
			game_network.connect_to_server = false;
			connect_to_server(&platform_network, &game_network);
		}
		if(game_network.connected)
		{
			enet_loop(platform_network.client, 0, &game_network, parse_packet_proc);
			if(game_network.disconnect)
			{
				enet_peer_disconnect(platform_network.server, 0);
				enet_loop(platform_network.client, 1000, &game_network, parse_packet_proc);
			}
		}

		(*update_game_proc)(platform_data, platform_funcs, &game_network, game_memory, false);
		platform_data.recompiled = false;

		for (int packet_i = 0; packet_i < game_network.out_packets.count; ++packet_i)
		{
			s_packet packet = game_network.out_packets[packet_i];
			ENetPacket* enet_packet = enet_packet_create(packet.data, packet.size, packet.flag);
			enet_peer_send(platform_network.server, 0, enet_packet);
		}
		game_network.out_packets.count = 0;
		game_network.read_arena.used = 0;
		game_network.write_arena.used = 0;

		glXSwapBuffers(g_window.display, g_window.window);

		time_passed = get_seconds(start_cycles) - start_of_frame_seconds;
	}
	return 0;
}
