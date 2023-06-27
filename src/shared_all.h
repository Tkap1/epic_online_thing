
global constexpr int c_max_peers = 32;
global constexpr u16 c_internal_server_port = 9417;
global constexpr int c_game_memory = 3 * c_mb;

struct s_packet
{
	int flag;
	int size;
	u8* data;

	#ifdef m_server
	b8 broadcast;
	u32 target;
	u32 from;
	#endif // m_server

};

#define m_parse_packet(name) void name(s_packet packet)
typedef m_parse_packet(t_parse_packet);