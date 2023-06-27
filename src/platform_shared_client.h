#ifdef _WIN32
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
#else
#define m_gl_funcs \
X(PFNGLBUFFERDATAPROC, glBufferDataProc) \
X(PFNGLBUFFERSUBDATAPROC, glBufferSubDataProc) \
X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArraysProc) \
X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArrayProc) \
X(PFNGLGENBUFFERSPROC, glGenBuffersProc) \
X(PFNGLBINDBUFFERPROC, glBindBufferProc) \
X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointerProc) \
X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArrayProc) \
X(PFNGLCREATESHADERPROC, glCreateShaderProc) \
X(PFNGLSHADERSOURCEPROC, glShaderSourceProc) \
X(PFNGLCREATEPROGRAMPROC, glCreateProgramProc) \
X(PFNGLATTACHSHADERPROC, glAttachShaderProc) \
X(PFNGLLINKPROGRAMPROC, glLinkProgramProc) \
X(PFNGLCOMPILESHADERPROC, glCompileShaderProc) \
X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisorProc) \
X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstancedProc) \
X(PFNGLDEBUGMESSAGECALLBACKPROC, glDebugMessageCallbackProc) \
X(PFNGLBINDBUFFERBASEPROC, glBindBufferBaseProc) \
X(PFNGLUNIFORM2FVPROC, glUniform2fvProc) \
X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocationProc) \
X(PFNGLUSEPROGRAMPROC, glUseProgramProc) \
X(PFNGLGETSHADERIVPROC, glGetShaderivProc) \
X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLogProc) \
X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffersProc) \
X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebufferProc) \
X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2DProc) \
X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatusProc) \
X(PFNGLACTIVETEXTUREPROC, glActiveTextureProc) \
X(PFNGLBLENDEQUATIONPROC, glBlendEquationProc) \
X(PFNGLDELETEPROGRAMPROC, glDeleteProgramProc) \
X(PFNGLDELETESHADERPROC, glDeleteShaderProc) \
X(PFNGLUNIFORM1FPROC, glUniform1fProc) \
X(PFNGLXSWAPINTERVALEXTPROC, glXSwapIntervalEXTProc)

#define glBufferData glBufferDataProc
#define glBufferSubData glBufferSubDataProc
#define glGenVertexArrays glGenVertexArraysProc
#define glBindVertexArray glBindVertexArrayProc
#define glGenBuffers glGenBuffersProc
#define glBindBuffer glBindBufferProc
#define glVertexAttribPointer glVertexAttribPointerProc
#define glEnableVertexAttribArray glEnableVertexAttribArrayProc
#define glCreateShader glCreateShaderProc
#define glShaderSource glShaderSourceProc
#define glCreateProgram glCreateProgramProc
#define glAttachShader glAttachShaderProc
#define glLinkProgram glLinkProgramProc
#define glCompileShader glCompileShaderProc
#define glVertexAttribDivisor glVertexAttribDivisorProc
#define glDrawArraysInstanced glDrawArraysInstancedProc
#define glDebugMessageCallback glDebugMessageCallbackProc
#define glBindBufferBase glBindBufferBaseProc
#define glUniform2fv glUniform2fvProc
#define glGetUniformLocation glGetUniformLocationProc
#define glUseProgram glUseProgramProc
#define glGetShaderiv glGetShaderivProc
#define glGetShaderInfoLog glGetShaderInfoLogProc
#define glGenFramebuffers glGenFramebuffersProc
#define glBindFramebuffer glBindFramebufferProc
#define glFramebufferTexture2D glFramebufferTexture2DProc
#define glCheckFramebufferStatus glCheckFramebufferStatusProc
#define glActiveTexture glActiveTextureProc
#define glBlendEquation glBlendEquationProc
#define glDeleteProgram glDeleteProgramProc
#define glDeleteShader glDeleteShaderProc
#define glUniform1f glUniform1fProc
#define glXCreateContextAttribsARB glXCreateContextAttribsARBProc
#define glXSwapIntervalEXT glXSwapIntervalEXTProc
#endif

#define X(type, name) extern type name;
m_gl_funcs
#undef X

global constexpr int c_key_backspace = 0x08;
global constexpr int c_key_tab = 0x09;
global constexpr int c_key_enter = 0x0D;
global constexpr int c_key_alt = 0x12;
global constexpr int c_key_escape = 0x1B;
global constexpr int c_key_space = 0x20;
global constexpr int c_key_end = 0x23;
global constexpr int c_key_home = 0x24;
global constexpr int c_key_left = 0x25;
global constexpr int c_key_up = 0x26;
global constexpr int c_key_right = 0x27;
global constexpr int c_key_down = 0x28;
global constexpr int c_key_delete = 0x2E;
global constexpr int c_key_0 = 0x30;
global constexpr int c_key_1 = 0x31;
global constexpr int c_key_2 = 0x32;
global constexpr int c_key_3 = 0x33;
global constexpr int c_key_4 = 0x34;
global constexpr int c_key_5 = 0x35;
global constexpr int c_key_6 = 0x36;
global constexpr int c_key_7 = 0x37;
global constexpr int c_key_8 = 0x38;
global constexpr int c_key_9 = 0x39;
global constexpr int c_key_a = 0x41;
global constexpr int c_key_b = 0x42;
global constexpr int c_key_c = 0x43;
global constexpr int c_key_d = 0x44;
global constexpr int c_key_e = 0x45;
global constexpr int c_key_f = 0x46;
global constexpr int c_key_g = 0x47;
global constexpr int c_key_h = 0x48;
global constexpr int c_key_i = 0x49;
global constexpr int c_key_j = 0x4A;
global constexpr int c_key_k = 0x4B;
global constexpr int c_key_l = 0x4C;
global constexpr int c_key_m = 0x4D;
global constexpr int c_key_n = 0x4E;
global constexpr int c_key_o = 0x4F;
global constexpr int c_key_p = 0x50;
global constexpr int c_key_q = 0x51;
global constexpr int c_key_r = 0x52;
global constexpr int c_key_s = 0x53;
global constexpr int c_key_t = 0x54;
global constexpr int c_key_u = 0x55;
global constexpr int c_key_v = 0x56;
global constexpr int c_key_w = 0x57;
global constexpr int c_key_x = 0x58;
global constexpr int c_key_y = 0x59;
global constexpr int c_key_z = 0x5A;
global constexpr int c_key_add = 0x6B;
global constexpr int c_key_subtract = 0x6D;
global constexpr int c_key_f1 = 0x70;
global constexpr int c_key_f2 = 0x71;
global constexpr int c_key_f3 = 0x72;
global constexpr int c_key_f4 = 0x73;
global constexpr int c_key_f5 = 0x74;
global constexpr int c_key_f6 = 0x75;
global constexpr int c_key_f7 = 0x76;
global constexpr int c_key_f8 = 0x77;
global constexpr int c_key_f9 = 0x78;
global constexpr int c_key_f10 = 0x79;
global constexpr int c_key_f11 = 0x7A;
global constexpr int c_key_f12 = 0x7B;
global constexpr int c_key_left_shift = 0xA0;
global constexpr int c_key_right_shift = 0xA1;
global constexpr int c_key_left_control = 0xA2;
global constexpr int c_key_right_control = 0xA3;
global constexpr int c_max_keys = 1024;

struct s_sound
{
	int sample_count;
	s16* samples;
};

typedef void* (*t_load_gl_func)(const char*);
typedef b8 (*t_play_sound)(s_sound);
typedef void (*t_set_swap_interval)(int);

struct s_game_window
{
	int width;
	int height;

	// @Note(tkap, 24/06/2023): Set by the game
	s_v2 size;
	s_v2 center;
};

struct s_char_event
{
	b8 is_symbol;
	int c;
};

struct s_stored_input
{
	b8 is_down;
	int key;
};

struct s_key
{
	b8 is_down;
	int count;
};

struct s_input
{
	s_key keys[c_max_keys];
};

struct s_platform_data
{
	b8 recompiled;
	b8 quit_after_this_frame;
	int window_width;
	int window_height;
	f64 time_passed;
	s_input* input;
	s_sarray<s_char_event, 1024>* char_event_arr;
	s_lin_arena frame_arena;
};

struct s_platform_funcs
{
	t_load_gl_func load_gl_func;
	t_play_sound play_sound;
	t_set_swap_interval set_swap_interval;
};

struct s_game_network
{
	int port;
	s_small_str ip;
	b8 connect_to_server;
	b8 connected;
	b8 disconnect;
	s_lin_arena read_arena;
	s_lin_arena write_arena;
	s_sarray<s_packet, 1024> out_packets;
};


#define m_update_game(name) void name(s_platform_data platform_data, s_platform_funcs platform_funcs, s_game_network* game_network, void* game_memory, b8 disgusting_recompile_hack)
typedef m_update_game(t_update_game);
