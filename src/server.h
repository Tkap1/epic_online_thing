

struct s_game
{
	b8 initialized;
	b8 last_restart_had_players;
	int level_count;
	int current_level;
	int attempt_count_on_current_level;
	float time_alive_packet_timer;
	f64 update_timer;
	s_rng rng;
	s_entities e;
};


func void update(void);
func void revive_every_player(void);

#define broadcast_packet(packet_id, data, flag) broadcast_packet_(packet_id, &data, sizeof(data), flag)
func void broadcast_packet_(e_packet packet_id, void* data, size_t size, int flag);

func void broadcast_simple_packet(e_packet packet_id, int flag);

#define send_packet(peer_id, packet_id, data, flag) send_packet_(peer_id, packet_id, &data, sizeof(data), flag)
func void send_packet_(u32 peer_id, e_packet packet_id, void* data, size_t size, int flag);