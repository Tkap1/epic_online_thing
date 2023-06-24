
// #define c_port 9417
#define c_port 9417


#ifdef m_client
#define handle_instant_movement(entity) handle_instant_movement_(entity)
#else // m_client
#define handle_instant_movement(entity)
#endif

typedef struct s_name
{
	int len;
	char data[max_player_name_length];
} s_name;


typedef enum e_packet
{
	e_packet_welcome,
	e_packet_already_connected_player,
	e_packet_another_player_connected,
	e_packet_player_update,
	e_packet_player_disconnected,
	e_packet_player_died,
	e_packet_start_level,
	e_packet_set_level,
	e_packet_beat_level,
	e_packet_reset_level,
	e_packet_player_got_hit,
	e_packet_player_name,
	e_packet_cheat_next_level,
	e_packet_cheat_previous_level,
} e_packet;

#pragma pack(push, 1)

/////

typedef struct s_welcome_from_server
{
	u32 id;
} s_welcome_from_server;

typedef struct s_welcome_from_client
{
	int unused;
} s_welcome_from_client;

typedef struct s_already_connected_player_from_server
{
	u32 id;
	b8 dead;
	s_name name;
} s_already_connected_player_from_server;

typedef struct s_already_connected_player_from_client
{
	int unused;
} s_already_connected_player_from_client;

typedef struct s_another_player_connected_from_server
{
	u32 id;
	b8 dead;
} s_another_player_connected_from_server;

typedef struct s_another_player_connected_from_client
{
	int unused;
} s_another_player_connected_from_client;

typedef struct s_player_update_from_server
{
	u32 id;
	float x;
	float y;
} s_player_update_from_server;

typedef struct s_player_update_from_client
{
	float x;
	float y;
} s_player_update_from_client;

typedef struct s_player_disconnected_from_server
{
	u32 id;
} s_player_disconnected_from_server;

typedef struct s_player_disconnected_from_client
{
	int unused;
} s_player_disconnected_from_client;

typedef struct s_player_died_from_server
{
	int unused;
} s_player_died_from_server;

typedef struct s_player_died_from_client
{
	int unused;
} s_player_died_from_client;

typedef struct s_start_level_from_server
{
	int unused;
} s_start_level_from_server;

typedef struct s_start_level_from_client
{
	int unused;
} s_start_level_from_client;

typedef struct s_set_level_from_server
{
	int unused;
} s_set_level_from_server;

typedef struct s_set_level_from_client
{
	int unused;
} s_set_level_from_client;

typedef struct s_beat_level_from_server
{
	int current_level;
	u32 seed;
} s_beat_level_from_server;

typedef struct s_beat_level_from_client
{
	int unused;
} s_beat_level_from_client;

typedef struct s_reset_level_from_server
{
	int current_level;
	u32 seed;
} s_reset_level_from_server;

typedef struct s_reset_level_from_client
{
	int unused;
} s_reset_level_from_client;

typedef struct s_player_got_hit_from_server
{
	u32 id;
} s_player_got_hit_from_server;

typedef struct s_player_got_hit_from_client
{
	int unused;
} s_player_got_hit_from_client;

typedef struct s_player_name_from_server
{
	u32 id;
	s_name name;
} s_player_name_from_server;

typedef struct s_player_name_from_client
{
	s_name name;
} s_player_name_from_client;

typedef struct s_cheat_previous_level_from_server
{
	int current_level;
	u32 seed;
} s_cheat_previous_level_from_server;


#pragma pack(pop)

typedef enum e_entity_flag
{
	e_entity_flag_move,
	e_entity_flag_player_movement,
	e_entity_flag_physics_movement,
	e_entity_flag_input,
	e_entity_flag_draw,
	e_entity_flag_draw_circle,
	e_entity_flag_gravity,
	e_entity_flag_player_bounds_check,
	e_entity_flag_projectile_bounds_check,
	e_entity_flag_expire,
	e_entity_flag_collide,
	e_entity_flag_projectile_spawner,
	e_entity_flag_count,
} e_entity_flag;

typedef enum e_entity_type
{
	e_entity_type_player,
	e_entity_type_projectile,
	e_entity_type_count,
} e_entity_type;

typedef struct s_entities
{
	int count;
	int next_id;

	b8 active[c_max_entities];
	b8 flags[c_max_entities][e_entity_flag_count];
	b8 jumping[c_max_entities];
	b8 dead[c_max_entities];
	b8 drawn_last_render[c_max_entities];
	e_entity_type type[c_max_entities];
	int id[c_max_entities];
	int jumps_done[c_max_entities];
	u32 player_id[c_max_entities];
	float prev_x[c_max_entities];
	float prev_y[c_max_entities];
	float x[c_max_entities];
	float y[c_max_entities];
	float sx[c_max_entities];
	float sy[c_max_entities];
	float dir_x[c_max_entities];
	float dir_y[c_max_entities];
	float vel_x[c_max_entities];
	float vel_y[c_max_entities];
	float speed[c_max_entities];
	float time_lived[c_max_entities];
	float duration[c_max_entities];
	float spawn_timer[c_max_entities];
	float spawn_delay[c_max_entities];
	s_v4 color[c_max_entities];
	s_name name[c_max_entities];
} s_entities;

typedef enum e_projectile_type
{
	e_projectile_type_top_basic,
	e_projectile_type_left_basic,
	e_projectile_type_right_basic,
	e_projectile_type_diagonal_left,
	e_projectile_type_diagonal_right,
	e_projectile_type_diagonal_bottom_left,
	e_projectile_type_diagonal_bottom_right,
	e_projectile_type_cross,
	e_projectile_type_spawner,
	e_projectile_type_count,
} e_projectile_type;

typedef struct s_level
{
	float spawn_delay[e_projectile_type_count];
	float speed_multiplier[e_projectile_type_count];
} s_level;

global s_level levels[c_max_levels];

func int make_entity();
func void zero_entity(int index);
func int find_player_by_id(u32 id);
func void gravity_system(int start, int count);
func int make_player(u32 player_id, b8 dead);
func void init_levels();
func int make_projectile();
func void expire_system(int start, int count);
func void make_diagonal_bottom_projectile(int entity, float x, float angle);
func void make_diagonal_top_projectile(int entity, float x, float opposite_x);
func void make_side_projectile(int entity, float x, float x_dir);
func s_name str_to_name(char* str);

#define send_packet(peer, packet_id, data, flag) send_packet_(peer, packet_id, &data, sizeof(data), flag)
func void send_packet_(ENetPeer* peer, e_packet packet_id, void* data, size_t size, int flag);

#define broadcast_packet(host, packet_id, data, flag) broadcast_packet_(host, packet_id, &data, sizeof(data), flag)
func void broadcast_packet_(ENetHost* in_host, e_packet packet_id, void* data, size_t size, int flag);