#define m_server 1
static constexpr int ENET_PACKET_FLAG_RELIABLE = 1;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#else
#include <x86intrin.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <stdio.h>
// #include <string.h>
#include <stdarg.h>
#include <math.h>
#include "types.h"
#include "memory.h"
#include "utils.h"
#include "epic_math.h"
#include "shared_all.h"
#include "shared_client_server.h"
#include "config.h"
#include "platform_shared_server.h"
#include "rng.h"
#include "shared.h"
#include "server.h"
#include "epic_time.h"

global s_lin_arena* frame_arena;
global s_game* game;
global s_game_network* g_network;

#include "shared.cpp"
#include "memory.cpp"


extern "C"
{

__declspec(dllexport)
m_update_game(update_game)
{
	static_assert(c_game_memory >= sizeof(s_game));

	frame_arena = &platform_data.frame_arena;
	g_network = game_network;
	game = (s_game*)game_memory;
	if(disgusting_recompile_hack) { return ; }

	if(!game->initialized)
	{
		game->initialized = true;
		init_levels();
		game->rng.seed = (u32)__rdtsc();
	}

	if(platform_data.recompiled)
	{
		init_levels();

		s_update_levels_from_server data = zero;
		memcpy(data.levels, levels, sizeof(levels));
		broadcast_packet(e_packet_update_levels, data, ENET_PACKET_FLAG_RELIABLE);
	}

	game->update_timer += platform_data.time_passed;
	while(game->update_timer >= c_update_delay)
	{
		game->update_timer -= c_update_delay;
		update();
	}

	frame_arena->used = 0;
}
}

func void update(void)
{
	for(int i = 0; i < c_num_threads; i++)
	{
		increase_time_lived_system(i * c_entities_per_thread, c_entities_per_thread);
	}
	for(int i = 0; i < c_num_threads; i++)
	{
		modify_speed_system(i * c_entities_per_thread, c_entities_per_thread);
		gravity_system(i * c_entities_per_thread, c_entities_per_thread);
	}
	for(int i = 0; i < c_num_threads; i++)
	{
		move_system(i * c_entities_per_thread, c_entities_per_thread);
		player_movement_system(i * c_entities_per_thread, c_entities_per_thread);
		physics_movement_system(i * c_entities_per_thread, c_entities_per_thread);
	}
	for(int i = 0; i < c_num_threads; i++)
	{
		player_bounds_check_system(i * c_entities_per_thread, c_entities_per_thread);
		projectile_bounds_check_system(i * c_entities_per_thread, c_entities_per_thread);
	}
	for(int i = 0; i < c_num_threads; i++)
	{
		projectile_spawner_system(i * c_entities_per_thread, c_entities_per_thread);
	}

	spawn_system(levels[game->current_level]);

	b8 at_least_one_player_alive = false;
	foreach_raw(peer_i, peer, g_network->peers)
	{
		int entity = find_player_by_id(peer);
		if(entity != c_invalid_entity && !game->e.dead[entity])
		{
			at_least_one_player_alive = true;
			break;
		}
	}
	level_timer += delta;
	if(level_timer >= levels[game->current_level].duration && at_least_one_player_alive)
	{
		s_beat_level_from_server data = zero;
		data.current_level = game->current_level;
		data.seed = game->rng.seed;
		broadcast_packet(e_packet_beat_level, data, ENET_PACKET_FLAG_RELIABLE);

		log("Level %i beaten", game->current_level + 1);

		game->current_level += 1;

		game->attempt_count_on_current_level = 0;

		// @Note(tkap, 25/06/2023): Reset if we go past all levels
		if(game->current_level >= game->level_count)
		{
			game->current_level = 0;
			broadcast_simple_packet(e_packet_all_levels_beat, ENET_PACKET_FLAG_RELIABLE);
		}
		reset_level();
		revive_every_player();
	}
	if(!at_least_one_player_alive)
	{
		log("Level %i restarted", game->current_level + 1);
		reset_level();
		revive_every_player();

		if(g_network->peers.count > 0)
		{
			if(game->last_restart_had_players)
			{
				game->attempt_count_on_current_level += 1;
			}
			game->last_restart_had_players = true;
		}
		else
		{
			game->last_restart_had_players = false;
		}

		s_reset_level_from_server data = zero;
		data.current_level = game->current_level;
		data.seed = game->rng.seed;
		data.attempt_count_on_current_level = game->attempt_count_on_current_level;
		broadcast_packet(e_packet_reset_level, data, ENET_PACKET_FLAG_RELIABLE);
	}

	game->time_alive_packet_timer += delta;
	if(game->time_alive_packet_timer >= 1)
	{
		game->time_alive_packet_timer -= 1;

		foreach_raw(peer_i, peer, g_network->peers)
		{
			int entity = find_player_by_id(peer);
			if(entity != c_invalid_entity)
			{
				s_update_time_lived_from_server data = zero;
				data.id = peer;
				data.time_lived = game->e.time_lived[entity];
				broadcast_packet(e_packet_update_time_lived, data, 0);
			}
		}
	}
}

extern "C"
{

__declspec(dllexport)
m_parse_packet(parse_packet)
{
	u8* cursor = packet.data;
	e_packet packet_id = *(e_packet*)buffer_read(&cursor, sizeof(packet_id));

	switch(packet_id)
	{
		case e_packet_connect:
		{
			log("%u connected!", packet.from);

			// @Note(tkap, 22/06/2023): Send all players to the new client
			foreach_raw(peer_i, peer, g_network->peers)
			{
				// @Note(tkap, 26/06/2023): Avoid sending the new client its own character
				if(peer == packet.from) { continue; }

				int entity = find_player_by_id(peer);
				assert(entity != c_invalid_entity);

				s_already_connected_player_from_server data = zero;
				data.id = peer;
				data.dead = game->e.dead[entity];
				data.name = game->e.name[entity];
				data.color = game->e.color[entity];
				send_packet(packet.from, e_packet_already_connected_player, data, ENET_PACKET_FLAG_RELIABLE);
				log("Sent already connected data of %u to %u", peer, packet.from);
			}

			// @Note(tkap, 22/06/2023): Welcome the new client
			{
				s_welcome_from_server data = zero;
				data.id = packet.from;
				data.current_level = game->current_level;
				data.seed = game->rng.seed;
				data.attempt_count_on_current_level = game->attempt_count_on_current_level;
				static_assert(sizeof(levels) == sizeof(data.levels));
				memcpy(data.levels, levels, sizeof(levels));
				send_packet(packet.from, e_packet_welcome, data, ENET_PACKET_FLAG_RELIABLE);
				log("Sent welcome to %u", packet.from);
			}

			// @Note(tkap, 22/06/2023): Send every other client the new character
			foreach_raw(peer_i, peer, g_network->peers)
			{
				if(peer == packet.from) { continue; }
				s_another_player_connected_from_server data = zero;
				data.id = packet.from;
				data.dead = true;
				send_packet(peer, e_packet_another_player_connected, data, ENET_PACKET_FLAG_RELIABLE);
				log("Sent player_connected of %u to %u", packet.from, peer);
			}

			make_player(packet.from, true, v41f(1));
		} break;

		case e_packet_disconnect:
		{
			foreach_raw(peer_i, peer, g_network->peers)
			{
				s_player_disconnected_from_server data = zero;
				data.id = packet.from;
				send_packet(peer, e_packet_player_disconnected, data, ENET_PACKET_FLAG_RELIABLE);
			}

			log("%u disconnected!", packet.from);
		} break;

		case e_packet_player_update:
		{
			s_player_update_from_client data = *(s_player_update_from_client*)cursor;

			// @TODO(tkap, 22/06/2023): We should probably check that the sender is an actual client?
			// @Note(tkap, 22/06/2023): Set the new player data to everyone else except the sender
			foreach_raw(peer_i, peer, g_network->peers)
			{
				if(peer == packet.from) { continue; }

				s_player_update_from_server out_data = zero;
				out_data.id = packet.from;
				out_data.x = data.x;
				out_data.y = data.y;
				send_packet(peer, e_packet_player_update, out_data, 0);
			}

			int entity = find_player_by_id(packet.from);
			if(entity != c_invalid_entity)
			{
				game->e.x[entity] = data.x;
				game->e.y[entity] = data.y;
			}

		} break;

		case e_packet_player_got_hit:
		{
			u32 got_hit_id = packet.from;
			int entity = find_player_by_id(got_hit_id);
			if(entity != c_invalid_entity)
			{
				game->e.dead[entity] = true;
			}

			foreach_raw(peer_i, peer, g_network->peers)
			{
				if(peer == got_hit_id) { continue; }

				s_player_got_hit_from_server data = zero;
				data.id = got_hit_id;
				send_packet(peer, e_packet_player_got_hit, data, ENET_PACKET_FLAG_RELIABLE);
			}

		} break;

		case e_packet_player_appearance:
		{
			u32 player_id = packet.from;
			s_player_appearance_from_client data = *(s_player_appearance_from_client*)cursor;

			if(data.name.len < 3 || data.name.len > data.name.max_chars)
			{
				// @TODO(tkap, 23/06/2023): Kick player?
				break;
			}

			int entity = find_player_by_id(player_id);
			if(entity != c_invalid_entity)
			{
				game->e.name[entity] = data.name;
				game->e.color[entity] = data.color;
				log("Set %u's name to %s", player_id, game->e.name[entity].data);

				// @Note(tkap, 23/06/2023): Send the name to everyone else
				foreach_raw(peer_i, peer, g_network->peers)
				{
					if(peer == player_id) { continue; }

					s_player_appearance_from_server out_data = zero;
					out_data.id = player_id;
					out_data.name = data.name;
					out_data.color = data.color;
					send_packet(peer, e_packet_player_appearance, out_data, ENET_PACKET_FLAG_RELIABLE);
				}
			}

		} break;

		#ifdef m_debug
		case e_packet_cheat_next_level:
		{
			// if(peers.count > 1) { break; }

			s_beat_level_from_server data = zero;
			data.current_level = game->current_level;
			data.seed = game->rng.seed;
			broadcast_packet(e_packet_beat_level, data, ENET_PACKET_FLAG_RELIABLE);

			game->attempt_count_on_current_level = 0;
			game->current_level += 1;
			reset_level();
			revive_every_player();
		} break;

		case e_packet_cheat_previous_level:
		{
			// if(peers.count > 1) { break; }
			if(game->current_level <= 0) { break; }

			game->attempt_count_on_current_level = 0;
			game->current_level -= 1;

			s_cheat_previous_level_from_server data = zero;
			data.current_level = game->current_level;
			data.seed = game->rng.seed;
			broadcast_packet(e_packet_cheat_previous_level, data, ENET_PACKET_FLAG_RELIABLE);

			reset_level();
			revive_every_player();
		} break;
		#endif // m_debug

		case e_packet_chat_msg:
		{
			s_chat_msg_from_client data = *(s_chat_msg_from_client*)cursor;
			if(data.msg.len <= 0 || data.msg.len > data.msg.max_chars) { break; }

			s_chat_msg_from_server out_data = zero;
			out_data.id = packet.from;
			out_data.msg = data.msg;
			broadcast_packet(e_packet_chat_msg, out_data, ENET_PACKET_FLAG_RELIABLE);
		} break;
	}
}
}

func void revive_every_player(void)
{
	foreach_raw(peer_i, peer, g_network->peers)
	{
		int entity = find_player_by_id(peer);
		if(entity != c_invalid_entity)
		{
			game->e.dead[entity] = false;
		}
	}
}

func void broadcast_packet_(e_packet packet_id, void* data, size_t size, int flag)
{
	assert(flag == 0 || flag == ENET_PACKET_FLAG_RELIABLE);

	s_packet packet = zero;
	packet.size = (int)(size + sizeof(packet_id));
	packet.data = (u8*)la_get(&g_network->write_arena, packet.size);
	packet.broadcast = true;
	packet.flag = flag;
	u8* cursor = packet.data;
	buffer_write(&cursor, &packet_id, sizeof(packet_id));
	buffer_write(&cursor, data, size);
	g_network->out_packets.add(packet);
}

func void broadcast_simple_packet(e_packet packet_id, int flag)
{
	assert(flag == 0 || flag == ENET_PACKET_FLAG_RELIABLE);

	s_packet packet = zero;
	packet.size = sizeof(packet_id);
	packet.data = (u8*)la_get(&g_network->write_arena, packet.size);
	packet.broadcast = true;
	packet.flag = flag;
	*(e_packet*)packet.data = packet_id;
	g_network->out_packets.add(packet);
}

func void send_packet_(u32 peer_id, e_packet packet_id, void* data, size_t size, int flag)
{
	assert(flag == 0 || flag == ENET_PACKET_FLAG_RELIABLE);

	s_packet packet = zero;
	packet.size = (int)(size + sizeof(packet_id));
	packet.data = (u8*)la_get(&g_network->write_arena, packet.size);
	packet.target = peer_id;
	packet.flag = flag;
	u8* cursor = packet.data;
	buffer_write(&cursor, &packet_id, sizeof(packet_id));
	buffer_write(&cursor, data, size);
	g_network->out_packets.add(packet);
}
