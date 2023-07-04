
struct s_game_network
{
	s_sarray<u32, c_max_peers> peers;
	s_lin_arena read_arena;
	s_lin_arena write_arena;
	s_sarray<s_packet, 1024> out_packets;
};

struct s_platform_data
{
	b8 recompiled;
	f64 time_passed;
	s_lin_arena* frame_arena;
	s_lin_arena* permanent_arena;
};


#define m_update_game(name) void name(s_platform_data platform_data, s_game_network* game_network, void* game_memory, b8 disgusting_recompile_hack)
typedef m_update_game(t_update_game);
