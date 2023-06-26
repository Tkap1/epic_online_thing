


// @Note(tkap, 26/06/2023): This exists because when a peer disconnects, its ID is cleared to 0 before we have a chance to look at it,
// so we create this to store a copy of the ID
struct s_peer
{
	u32 id;
	ENetPeer* peer;
};

struct s_platform_network
{
	ENetHost* host;
 	s_sarray<s_peer, c_max_peers> peers;
};


func void enet_loop(s_platform_network* platform_network, s_game_network* game_network, t_parse_packet* parse_packet);