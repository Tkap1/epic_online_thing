

global constexpr u16 c_internal_server_port = 9417;

#define m_parse_packet(name) void name(s_packet packet)
typedef m_parse_packet(t_parse_packet);