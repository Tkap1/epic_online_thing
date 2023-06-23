
#define key_backspace 0x08
#define key_tab 0x09
#define key_enter 0x0D
#define key_alt 0x12
#define key_escape 0x1B
#define key_space 0x20
#define key_end 0x23
#define key_home 0x24
#define key_left 0x25
#define key_up 0x26
#define key_right 0x27
#define key_down 0x28
#define key_delete 0x2E

#define key_0 0x30
#define key_1 0x31
#define key_2 0x32
#define key_3 0x33
#define key_4 0x34
#define key_5 0x35
#define key_6 0x36
#define key_7 0x37
#define key_8 0x38
#define key_9 0x39
#define key_a 0x41
#define key_b 0x42
#define key_c 0x43
#define key_d 0x44
#define key_e 0x45
#define key_f 0x46
#define key_g 0x47
#define key_h 0x48
#define key_i 0x49
#define key_j 0x4A
#define key_k 0x4B
#define key_l 0x4C
#define key_m 0x4D
#define key_n 0x4E
#define key_o 0x4F
#define key_p 0x50
#define key_q 0x51
#define key_r 0x52
#define key_s 0x53
#define key_t 0x54
#define key_u 0x55
#define key_v 0x56
#define key_w 0x57
#define key_x 0x58
#define key_y 0x59
#define key_z 0x5A
#define key_add 0x6B
#define key_subtract 0x6D

#define key_f1 0x70
#define key_f2 0x71
#define key_f3 0x72
#define key_f4 0x73
#define key_f5 0x74
#define key_f6 0x75
#define key_f7 0x76
#define key_f8 0x77
#define key_f9 0x78
#define key_f10 0x79
#define key_f11 0x7A
#define key_f12 0x7B

#define key_left_shift 0xA0
#define key_right_shift 0xA1
#define key_left_control 0xA2
#define key_right_control 0xA3

typedef enum e_state
{
	e_state_main_menu,
	e_state_game,
	e_state_count,
} e_state;

typedef enum e_font
{
	e_font_small,
	e_font_medium,
	e_font_big,
	e_font_count,
} e_font;

typedef struct s_glyph
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
} s_glyph;

typedef struct s_texture
{
	u32 id;
	s_v2 size;
	s_v2 sub_size;
} s_texture;

typedef struct s_font
{
	float size;
	float scale;
	int ascent;
	int descent;
	int line_gap;
	s_texture texture;
	s_glyph glyph_arr[1024];
} s_font;

typedef struct s_main_menu
{
	int player_name_length;
	char* error_str;
	char player_name[max_player_name_length];
} s_main_menu;



func void update();
func void render(u32 program);
func b8 check_for_shader_errors(u32 id, char* out_error);
func void input_system(int start, int count);
func void draw_system(int start, int count);
func void parse_packet(ENetEvent event);
func void enet_loop(ENetHost* client, int timeout);
func void revive_every_player();
func void draw_circle_system(int start, int count);
func void collision_system(int start, int count);
func s_font load_font(char* path, float font_size, s_lin_arena* arena);
func s_texture load_texture_from_data(void* data, int width, int height, u32 filtering);
func s_v2 get_text_size(char* text, e_font font_id);
func s_v2 get_text_size_with_count(char* text, e_font font_id, int count);
func void connect_to_server();