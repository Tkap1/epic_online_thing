
#define c_port (9417)


#ifdef m_client
#define handle_instant_movement(entity) handle_instant_movement_(entity)
#define handle_instant_resize(entity) handle_instant_resize_(entity)
#else // m_client
#define handle_instant_movement(entity)
#define handle_instant_resize(entity)
#endif

enum e_packet
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
	e_packet_player_appearance,
	e_packet_cheat_next_level,
	e_packet_cheat_previous_level,
	e_packet_all_levels_beat,
	e_packet_update_time_lived,

	e_packet_connect = 9998,
	e_packet_disconnect = 9999,
};

#pragma pack(push, 1)

struct s_welcome_from_server
{
	u32 id;
	int current_level;
	u32 seed;
	int attempt_count_on_current_level;
};

struct s_welcome_from_client
{
	int unused;
};

struct s_already_connected_player_from_server
{
	u32 id;
	b8 dead;
	s_name name;
	s_v4 color;
};

struct s_already_connected_player_from_client
{
	int unused;
};

struct s_another_player_connected_from_server
{
	u32 id;
	b8 dead;
};

struct s_another_player_connected_from_client
{
	int unused;
};

struct s_player_update_from_server
{
	u32 id;
	float x;
	float y;
};

struct s_player_update_from_client
{
	float x;
	float y;
};

struct s_player_disconnected_from_server
{
	u32 id;
};

struct s_player_disconnected_from_client
{
	int unused;
};

struct s_player_died_from_server
{
	int unused;
};

struct s_player_died_from_client
{
	int unused;
};

struct s_start_level_from_server
{
	int unused;
};

struct s_start_level_from_client
{
	int unused;
};

struct s_set_level_from_server
{
	int unused;
};

struct s_set_level_from_client
{
	int unused;
};

struct s_beat_level_from_server
{
	int current_level;
	u32 seed;
};

struct s_beat_level_from_client
{
	int unused;
};

struct s_reset_level_from_server
{
	int current_level;
	u32 seed;
	int attempt_count_on_current_level;
};

struct s_reset_level_from_client
{
	int unused;
};

struct s_player_got_hit_from_server
{
	u32 id;
};

struct s_player_got_hit_from_client
{
	int unused;
};

struct s_player_appearance_from_server
{
	u32 id;
	s_name name;
	s_v4 color;
};

struct s_player_appearance_from_client
{
	s_name name;
	s_v4 color;
};

struct s_cheat_previous_level_from_server
{
	int current_level;
	u32 seed;
};

struct s_update_time_lived_from_server
{
	u32 id;
	float time_lived;
};

#pragma pack(pop)

enum e_particle_spawner_type
{
	e_particle_spawner_default, //Not sure what to call the random 2 blob spawner thingamajig
	e_particle_spawner_cross,
	e_particle_spawner_x,
	e_particle_spawner_type_count,
};

struct s_particle_spawner
{
	e_particle_spawner_type type;
	float spawn_timer;
	float spawn_delay;
};

enum e_entity_flag
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
	e_entity_flag_increase_time_lived,
	e_entity_flag_modify_speed,
	e_entity_flag_count,
};

enum e_entity_type
{
	e_entity_type_player,
	e_entity_type_projectile,
	e_entity_type_count,
};

enum e_curve_type
{
	e_curve_type_linear,
	e_curve_type_count,
};

struct s_float_curve
{
	e_curve_type type[4];
	float start_seconds[4];
	float end_seconds[4];
	float multiplier[4];
};

struct s_entities
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
	float prev_sx[c_max_entities];
	float prev_sy[c_max_entities];
	float sx[c_max_entities];
	float sy[c_max_entities];
	float dir_x[c_max_entities];
	float dir_y[c_max_entities];
	float vel_x[c_max_entities];
	float vel_y[c_max_entities];
	float base_speed[c_max_entities];
	float modified_speed[c_max_entities];
	float time_lived[c_max_entities];
	float duration[c_max_entities];
	s_particle_spawner particle_spawner[c_max_entities];
	s_float_curve speed_curve[c_max_entities];
	s_v4 color[c_max_entities];
	s_name name[c_max_entities];
};

enum e_projectile_type
{
	e_projectile_type_top_basic,
	e_projectile_type_left_basic,
	e_projectile_type_right_basic,
	e_projectile_type_diagonal_left,
	e_projectile_type_diagonal_right,
	e_projectile_type_diagonal_bottom_left,
	e_projectile_type_diagonal_bottom_right,
	e_projectile_type_corner_shot,
	e_projectile_type_shockwave,
	e_projectile_type_cross,
	e_projectile_type_spawner,
	e_projectile_type_count,
};

struct s_projectile_spawn_data
{
	e_projectile_type type;
	float delay;
	float speed_multiplier = 1;
	float size_multiplier = 1;
	s_float_curve speed_curve;
};

struct s_level
{
	int duration;
	s_sarray<s_projectile_spawn_data, c_max_spawns_per_level> spawn_data;
};

global s_level levels[c_max_levels];

func int make_entity(void);
func void zero_entity(int index);
func int find_player_by_id(u32 id);
func void gravity_system(int start, int count);
func int make_player(u32 player_id, b8 dead, s_v4 color);
func void init_levels(void);
func void expire_system(int start, int count);
func void make_diagonal_bottom_projectile(int entity, float x, float angle);
func void make_diagonal_top_projectile(int entity, float x, float opposite_x);
func void make_side_projectile(int entity, float x, float x_dir);
func s_name str_to_name(char* str);
func int make_entity(void);
func void init_levels(void);

func void apply_projectile_modifiers(int entity, s_projectile_spawn_data data);
func void set_speed(int entity, float speed);