
struct s_game
{
	float time_alive_packet_timer;
};


func void update(void);
func void parse_packet(ENetEvent event);
func void revive_every_player(void);