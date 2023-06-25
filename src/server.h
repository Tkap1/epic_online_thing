
struct s_game
{
	b8 last_restart_had_players;
	int attempt_count_on_current_level;

	float time_alive_packet_timer;
};


func void update(void);
func void parse_packet(ENetEvent event);
func void revive_every_player(void);