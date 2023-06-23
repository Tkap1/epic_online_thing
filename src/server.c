#define m_server 1

#include <winsock2.h>
#include <stdio.h>
#include "external\enet\enet.h"
#include <intrin.h>
#include <math.h>
#include "types.h"
#include "memory.h"
#include "utils.h"
#include "math.h"
#include "config.h"
#include "types.h"
#include "shared.h"
#include "time.h"
#include "rng.h"
#include "server.h"

#define c_max_peers 32

make_list(s_peer_list, ENetPeer*, c_max_peers)
global s_peer_list peers;
global s_lin_arena frame_arena;
global s_entities e;
global u32 peer_ids[c_max_peers];
global ENetHost* host;
global s_rng rng;

#include "shared.c"
#include "memory.c"


int main(int argc, char** argv)
{
	unreferenced(argc);
	unreferenced(argv);

	init_levels();
	rng.seed = (u32)__rdtsc();

	init_performance();
	frame_arena = make_lin_arena(1 * c_mb);

	if(enet_initialize() != 0)
	{
		error(false);
	}

	ENetAddress address = zero;
	address.host = ENET_HOST_ANY;
	address.port = c_port;

	host = enet_host_create(
		&address, /* create a client host */
		c_max_peers, /* only allow 1 outgoing connection */
		2, /* allow up 2 channels to be used, 0 and 1 */
		0, /* assume any amount of incoming bandwidth */
		0 /* assume any amount of outgoing bandwidth */
	);

	if(host == null)
	{
		error(false);
	}

	f64 update_timer = 0;

	while(true)
	{
		f64 start_of_frame_seconds = get_seconds();

		ENetEvent event = zero;
		while(enet_host_service(host, &event, 0) > 0)
		{
			switch(event.type)
			{
				case ENET_EVENT_TYPE_NONE:
				{
				} break;

				case ENET_EVENT_TYPE_CONNECT:
				{
					printf("Someone connected!\n");

					if(peers.count >= c_max_peers) { break; }

					// @Note(tkap, 22/06/2023): Send all players to the new client
					for(int peer_i = 0; peer_i < peers.count; peer_i++)
					{
						u32 id = peers.elements[peer_i]->connectID;
						int entity = find_player_by_id(id);
						assert(entity != c_invalid_entity);

						s_already_connected_player_from_server data = zero;
						data.id = id;
						data.dead = e.dead[entity];
						data.name = e.name[entity];
						send_packet(event.peer, e_packet_already_connected_player, data, ENET_PACKET_FLAG_RELIABLE);
					}

					// @Note(tkap, 22/06/2023): Welcome the new client
					{
						s_welcome_from_server data = zero;
						data.id = event.peer->connectID;
						send_packet(event.peer, e_packet_welcome, data, ENET_PACKET_FLAG_RELIABLE);
					}

					// @Note(tkap, 22/06/2023): Send every other client the new character
					for(int peer_i = 0; peer_i < peers.count; peer_i++)
					{
						ENetPeer* peer = peers.elements[peer_i];

						s_another_player_connected_from_server data = zero;
						data.id = event.peer->connectID;
						data.dead = true;
						send_packet(peer, e_packet_another_player_connected, data, ENET_PACKET_FLAG_RELIABLE);
					}

					s_peer_list_add(&peers, event.peer);
					peer_ids[peers.count - 1] = event.peer->connectID;

					make_player(event.peer->connectID, true);

				} break;

				case ENET_EVENT_TYPE_DISCONNECT:
				{
					// @Note(tkap, 22/06/2023): Find out the ID of the peer that disconnected, because for some reason, enet seems to set it to 0 before we have
					// a chance to read it.
					u32 id = 0;
					for(int peer_i = 0; peer_i < peers.count; peer_i++)
					{
						ENetPeer* peer = peers.elements[peer_i];
						if(peer->connectID == 0)
						{
							id = peer_ids[peer_i];
						}
					}
					assert(id != 0);

					for(int peer_i = 0; peer_i < peers.count; peer_i++)
					{
						ENetPeer* peer = peers.elements[peer_i];
						if(event.peer->connectID == peer->connectID)
						{
							peers.count -= 1;
							int to_move = peers.count - peer_i;
							if(to_move > 0)
							{
								memmove(&peers.elements[peer_i], &peers.elements[peer_i + 1], sizeof(peers.elements[0]) * to_move);
								memmove(&peer_ids[peer_i], &peer_ids[peer_i + 1], sizeof(peer_ids[0]) * to_move);
							}
						}
					}

					for(int peer_i = 0; peer_i < peers.count; peer_i++)
					{
						ENetPeer* peer = peers.elements[peer_i];
						s_player_disconnected_from_server data = zero;
						data.id = id;
						send_packet(peer, e_packet_player_disconnected, data, ENET_PACKET_FLAG_RELIABLE);
					}

					printf("Someone disconnected!\n");

				} break;

				case ENET_EVENT_TYPE_RECEIVE:
				{
					parse_packet(event);
					enet_packet_destroy(event.packet);
				} break;

				invalid_default_case;
			}
		}

		update_timer += time_passed;
		while(update_timer >= c_update_delay)
		{
			update_timer -= c_update_delay;
			update();
		}

		frame_arena.used = 0;

		time_passed = (float)(get_seconds() - start_of_frame_seconds);
		total_time += time_passed;
	}

	return 0;
}

func void update()
{
	spawn_system(levels[current_level]);

	b8 at_least_one_player_alive = false;
	for(int peer_i = 0; peer_i < peers.count; peer_i++)
	{
		ENetPeer* peer = peers.elements[peer_i];
		int entity = find_player_by_id(peer->connectID);
		if(entity != c_invalid_entity && !e.dead[entity])
		{
			at_least_one_player_alive = true;
			break;
		}
	}
	level_timer += delta;
	if(level_timer >= c_level_duration && at_least_one_player_alive)
	{
		s_beat_level_from_server data = zero;
		data.current_level = current_level;
		data.seed = rng.seed;
		broadcast_packet(host, e_packet_beat_level, data, ENET_PACKET_FLAG_RELIABLE);

		log("Level %i beaten", current_level + 1);

		current_level += 1;
		reset_level();
		revive_every_player();
		level_timer = 0;
	}
	if(!at_least_one_player_alive)
	{
		log("Level %i restarted", current_level + 1);
		reset_level();
		revive_every_player();

		s_reset_level_from_server data = zero;
		data.current_level = current_level;
		data.seed = rng.seed;
		broadcast_packet(host, e_packet_reset_level, data, ENET_PACKET_FLAG_RELIABLE);
	}

	for(int i = 0; i < c_num_threads; i++)
	{
		move_system(i * c_entities_per_thread, c_entities_per_thread);
		player_movement_system(i * c_entities_per_thread, c_entities_per_thread);
		physics_movement_system(i * c_entities_per_thread, c_entities_per_thread);
	}
	for(int i = 0; i < c_num_threads; i++)
	{
		bounds_check_system(i * c_entities_per_thread, c_entities_per_thread);
	}
	for(int i = 0; i < c_num_threads; i++)
	{
		projectile_spawner_system(i * c_entities_per_thread, c_entities_per_thread);
	}
}

func void parse_packet(ENetEvent event)
{
	u8* cursor = event.packet->data;
	e_packet packet_id = *(e_packet*)buffer_read(&cursor, sizeof(packet_id));

	switch(packet_id)
	{

		case e_packet_player_update:
		{
			s_player_update_from_client data = *(s_player_update_from_client*)cursor;

			// @TODO(tkap, 22/06/2023): We should probably check that the sender is an actual client?
			// @Note(tkap, 22/06/2023): Set the new player data to everyone else except the sender
			for(int peer_i = 0; peer_i < peers.count; peer_i++)
			{
				ENetPeer* peer = peers.elements[peer_i];
				if(peer->connectID == event.peer->connectID) { continue; }

				s_player_update_from_server out_data = zero;
				out_data.id = event.peer->connectID;
				out_data.x = data.x;
				out_data.y = data.y;
				send_packet(peer, e_packet_player_update, out_data, 0);
			}

			int entity = find_player_by_id(event.peer->connectID);
			if(entity != c_invalid_entity)
			{
				e.x[entity] = data.x;
				e.y[entity] = data.y;
			}

		} break;

		case e_packet_player_got_hit:
		{
			u32 got_hit_id = event.peer->connectID;
			int entity = find_player_by_id(got_hit_id);
			if(entity != c_invalid_entity)
			{
				e.dead[entity] = true;
			}

			for(int peer_i = 0; peer_i < peers.count; peer_i++)
			{
				ENetPeer* peer = peers.elements[peer_i];
				if(peer->connectID == got_hit_id) { continue; }

				s_player_got_hit_from_server data = zero;
				data.id = got_hit_id;
				send_packet(peer, e_packet_player_got_hit, data, ENET_PACKET_FLAG_RELIABLE);
			}

		} break;

		case e_packet_player_name:
		{
			u32 player_id = event.peer->connectID;
			s_player_name_from_client data = *(s_player_name_from_client*)cursor;

			if(data.name.len <= 3 || data.name.len > max_player_name_length)
			{
				// @TODO(tkap, 23/06/2023): Kick player?
				break;
			}

			int entity = find_player_by_id(player_id);
			if(entity != c_invalid_entity)
			{
				e.name[entity] = data.name;
				log("Set %u's name to %s", player_id, e.name[entity].data);

				// @Note(tkap, 23/06/2023): Send the name to everyone else
				for(int peer_i = 0; peer_i < peers.count; peer_i++)
				{
					ENetPeer* peer = peers.elements[peer_i];
					if(peer->connectID == player_id) { continue; }

					s_player_name_from_server out_data = zero;
					out_data.id = player_id;
					out_data.name = data.name;
					send_packet(peer, e_packet_player_name, out_data, ENET_PACKET_FLAG_RELIABLE);
				}
			}

		} break;
	}
}

func void revive_every_player()
{
	for(int peer_i = 0; peer_i < peers.count; peer_i++)
	{
		ENetPeer* peer = peers.elements[peer_i];
		int entity = find_player_by_id(peer->connectID);
		if(entity != c_invalid_entity)
		{
			e.dead[entity] = false;
		}
	}
}