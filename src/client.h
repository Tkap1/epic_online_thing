enum e_string_input_result
{
	e_string_input_result_none,
	e_string_input_result_submit,
};

enum e_state
{
	e_state_main_menu,
	e_state_game,
	e_state_count,
};

enum e_font
{
	e_font_small,
	e_font_medium,
	e_font_big,
	e_font_count,
};

struct s_glyph
{
	int advance_width;
	int width;
	int height;
	int x0;
	int y0;
	int x1;
	int y1;
	s_v2 uv_min;
	s_v2 uv_max;
};

struct s_texture
{
	u32 id;
	s_v2 size;
	s_v2 sub_size;
};

struct s_font
{
	float size;
	float scale;
	int ascent;
	int descent;
	int line_gap;
	s_texture texture;
	s_glyph glyph_arr[1024];
};

struct s_main_menu
{
	const char* error_str;
	s_small_str player_name;
};

struct s_config
{
	s_small_str player_name;
	s_small_str ip;
	int port;
	s_v4 color;
};


enum e_shader
{
	e_shader_default,
	e_shader_trail,
	e_shader_count,
};

struct s_shader_paths
{
	#ifdef m_debug
	#ifdef _WIN32
	FILETIME last_write_time;
	#else
	time_t last_write_time;
	#endif // _WIN32
	#endif // m_debug
	char* vertex_path;
	char* fragment_path;
};

struct s_trail_point
{
	float time;
	s_v2 a;
	s_v2 b;
	s_v4 color;
};

struct s_trail_point_gpu
{
	float x;
	float y;
	float distance_to_head;
	s_v4 color;
};


struct s_game
{
	b8 initialized;
	int level_count;
	int current_level;
	u32 my_id;
	int attempt_count_on_current_level;
	float total_time;
	float time_on_current_level;
	f64 update_timer;
	s_config config;
	s_rng rng;
	e_state state;
	s_font font_arr[e_font_count];
	s_main_menu main_menu;

	u32 default_vao;
	u32 default_ssbo;

	u32 programs[e_shader_count];

	u32 trail_vao;
	u32 trail_vbo;
	s_entities e;

	s_sound big_dog_sound;
	s_sound jump_sound;
	s_sound jump2_sound;
	s_sound win_sound;

	b8 chatting;
	s_medium_str my_chat_msg;

	// @Note(tkap, 27/06/2023): id of 0 means the message is not active
	u32 chat_message_ids[c_max_peers];
	float chat_message_times[c_max_peers];
	s_medium_str chat_messages[c_max_peers];
};

func void update(s_config config);
func void render(float dt);
func b8 check_for_shader_errors(u32 id, char* out_error);
func void input_system(int start, int count);
func void draw_system(int start, int count, float dt);
// func void enet_loop(ENetHost* client, int timeout, s_config config);
func void revive_every_player(void);
func void draw_circle_system(int start, int count, float dt);
func void collision_system(int start, int count);
func s_font load_font(const char* path, float font_size, s_lin_arena* arena);
func s_texture load_texture_from_data(void* data, int width, int height, u32 filtering);
func s_v2 get_text_size(const char* text, e_font font_id);
func s_v2 get_text_size_with_count(const char* text, e_font font_id, int count);
func void connect_to_server(s_config config);
func u32 load_shader(const char* vertex_path, const char* fragment_path);
func void handle_instant_movement_(int entity);
func void handle_instant_resize_(int entity);
func s_config make_default_config(s_rng* in_rng);
func s_small_str make_name(const char* str);
func void save_config(s_config config);
func s_config read_config_or_make_default(s_lin_arena* arena, s_rng* in_rng);
func b8 is_key_down(int key);
func b8 is_key_up(int key);
func b8 is_key_pressed(int key);
func b8 is_key_released(int key);
func s_char_event get_char_event();
void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
func void sine_alpha_system(int start, int count);

#define send_packet(packet_id, data, flag) send_packet_(packet_id, &data, sizeof(data), flag)
func void send_simple_packet(e_packet packet_id, int flag);
func void send_packet_(e_packet packet_id, void* data, size_t size, int flag);
func void connect_to_server(s_config config);

template <typename T>
func e_string_input_result handle_string_input(T* str);

#ifdef m_debug
func void hot_reload_shaders(void);
#endif // m_debug

func void add_trail(s_v2 a, s_v2 b, s_v2 in_size, s_v4 color);
func void trail_timer_system(int start, int count);