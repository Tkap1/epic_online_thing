
// #define c_port 9417
#define c_port 9417

#define begin_packet(packet_id) { \
	u8* packet_data = la_get(&frame_arena, 1024); \
	u8* write_cursor = packet_data; \
	e_packet packet_id_to_send = packet_id; \
	buffer_write(&write_cursor, &packet_id_to_send, sizeof(packet_id_to_send));

#define send_packet_peer(peer, flag) \
	ENetPacket* packet = enet_packet_create(packet_data, write_cursor - packet_data, flag); \
	enet_peer_send(peer, 0, packet); \
}

#define broadcast_packet(host, flag) \
	ENetPacket* packet = enet_packet_create(packet_data, write_cursor - packet_data, flag); \
	enet_host_broadcast(host, 0, packet); \
}

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
} e_packet;

#pragma pack(push, 1)
typedef struct s_packet_welcome
{
	u32 id;
} s_packet_welcome;

typedef struct s_already_connected_player
{
	u32 id;
	b8 dead;
	s_name name;
} s_already_connected_player;

typedef struct s_another_player_connected
{
	u32 id;
} s_another_player_connected;

typedef struct s_player_name_from_server
{
	u32 id;
	s_name name;
} s_player_name_from_server;

typedef struct s_player_name_from_client
{
	s_name name;
} s_player_name_from_client;
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
	e_entity_flag_bounds_check,
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
	e_entity_type type[c_max_entities];
	int id[c_max_entities];
	int jumps_done[c_max_entities];
	u32 player_id[c_max_entities];
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
	e_projectile_type_spawner,
	e_projectile_type_count,
} e_projectile_type;

typedef struct s_level
{
	float spawn_delay[e_projectile_type_count];
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