#define m_server 1

#include <winsock2.h>
#include <stdio.h>
#include "external\enet\enet.h"
#include "types.h"
#include "memory.h"
#include "utils.h"
#include "math.h"
#include "config.h"
#include "types.h"
#include "shared.h"
#include "time.h"
#include "server.h"

#define c_max_peers 32

make_list(s_peer_list, ENetPeer*, c_max_peers);
global s_peer_list peers;
global s_lin_arena frame_arena;
global s_entities e;

#include "shared.c"
#include "memory.c"


int main(int argc, char** argv)
{
	argc -= 1;
	argv += 1;

	init_performance();
	frame_arena = make_lin_arena(1 * c_mb);

	if(enet_initialize() != 0)
	{
		error(false);
	}

	ENetAddress address = zero;
	address.host = ENET_HOST_ANY;
	address.port = c_port;

	ENetHost* host = enet_host_create(
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
						la_push(&frame_arena);
						u32 id = peers.elements[peer_i]->connectID;
						e_packet packet_id = e_packet_another_player_connected;
						u8* data = la_get(&frame_arena, 1024);
						u8* cursor = data;
						buffer_write(&cursor, &packet_id, sizeof(packet_id));
						buffer_write(&cursor, &id, sizeof(id));
						ENetPacket* packet = enet_packet_create(data, cursor - data, ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(event.peer, 0, packet);
						la_pop(&frame_arena);
					}

					// @Note(tkap, 22/06/2023): Welcome the new client
					{
						la_push(&frame_arena);
						e_packet packet_id = e_packet_welcome;
						u8* data = la_get(&frame_arena, 1024);
						u8* cursor = data;
						buffer_write(&cursor, &packet_id, sizeof(packet_id));
						buffer_write(&cursor, &event.peer->connectID, sizeof(event.peer->connectID));
						ENetPacket* packet = enet_packet_create(data, cursor - data, ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(event.peer, 0, packet);
						la_pop(&frame_arena);
					}

					// @Note(tkap, 22/06/2023): Send every other client the new character
					for(int peer_i = 0; peer_i < peers.count; peer_i++)
					{
						la_push(&frame_arena);
						ENetPeer* peer = peers.elements[peer_i];
						e_packet packet_id = e_packet_another_player_connected;
						u8* data = la_get(&frame_arena, 1024);
						u8* cursor = data;
						buffer_write(&cursor, &packet_id, sizeof(packet_id));
						buffer_write(&cursor, &event.peer->connectID, sizeof(event.peer->connectID));
						ENetPacket* packet = enet_packet_create(data, cursor - data, ENET_PACKET_FLAG_RELIABLE);
						enet_peer_send(peer, 0, packet);
						la_pop(&frame_arena);
					}

					s_peer_list_add(&peers, event.peer);

					make_player(event.peer->connectID);

				} break;

				case ENET_EVENT_TYPE_DISCONNECT:
				{
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

		update_timer += delta;
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
	for(int i = 0; i < c_num_threads; i++)
	{
		move_system(i * c_entities_per_thread, c_entities_per_thread);
	}
	for(int i = 0; i < c_num_threads; i++)
	{
		bounds_check_system(i * c_entities_per_thread, c_entities_per_thread);
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
			float x = *(float*)buffer_read(&cursor, sizeof(float));
			float y = *(float*)buffer_read(&cursor, sizeof(float));

			// @TODO(tkap, 22/06/2023): We should probably check that the sender is an actual client?
			// @Note(tkap, 22/06/2023): Set the new player data to everyone else except the sender
			for(int peer_i = 0; peer_i < peers.count; peer_i++)
			{
				ENetPeer* peer = peers.elements[peer_i];
				if(peer->connectID == event.peer->connectID) { continue; }

				la_push(&frame_arena);
				e_packet packet_id_to_send = e_packet_player_update;
				u8* data = la_get(&frame_arena, 1024);
				u8* write_cursor = data;
				buffer_write(&write_cursor, &packet_id_to_send, sizeof(packet_id_to_send));
				buffer_write(&write_cursor, &event.peer->connectID, sizeof(event.peer->connectID));
				buffer_write(&write_cursor, &x, sizeof(x));
				buffer_write(&write_cursor, &y, sizeof(y));
				ENetPacket* packet = enet_packet_create(data, write_cursor - data, 0);
				enet_peer_send(peer, 0, packet);
				la_pop(&frame_arena);
			}

			int entity = find_player_by_id(event.peer->connectID);
			if(entity != c_invalid_entity)
			{
				e.x[entity] = x;
				e.y[entity] = y;
			}

		} break;
	}
}