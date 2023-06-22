
// #define c_port 9417
#define c_port 9417

typedef enum e_packet
{
	e_packet_welcome,
	e_packet_another_player_connected,
	e_packet_player_update,
} e_packet;

typedef enum e_entity_flag
{
	e_entity_flag_move,
	e_entity_flag_input,
	e_entity_flag_draw,
	e_entity_flag_count,
} e_entity_flag;

typedef struct s_entities
{
	int count;
	int next_id;

	b8 active[c_max_entities];
	b8 flags[c_max_entities][e_entity_flag_count];
	int id[c_max_entities];
	u32 player_id[c_max_entities];
	float x[c_max_entities];
	float y[c_max_entities];
	float sx[c_max_entities];
	float sy[c_max_entities];
	float dir_x[c_max_entities];
	float dir_y[c_max_entities];
	float speed[c_max_entities];
} s_entities;

func int make_entity();
func void zero_entity(int index);
func int find_player_by_id(u32 id);