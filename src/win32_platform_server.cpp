
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include "external/enet/enet.h"
#include <stdio.h>

#include "types.h"
#include "utils.h"
#include "memory.h"
#include "epic_time.h"
#include "platform_shared_server.h"
#include "shared_all.h"
#include "shared_client_server.h"
#include "win32_platform_server.h"
#include "platform_shared.h"

#include "memory.cpp"


int main(int argc, char** argv)
{
	unreferenced(argc);
	unreferenced(argv);

	init_performance();
	// frame_arena = make_lin_arena(1 * c_mb);

	if(enet_initialize() != 0)
	{
		error(false);
	}

	ENetAddress address = zero;
	address.host = ENET_HOST_ANY;
	address.port = 9417;

	s_platform_network platform_network = zero;
	s_game_network game_network = zero;

	platform_network.host = enet_host_create(
		&address, /* create a client host */
		c_max_peers, /* only allow 1 outgoing connection */
		2, /* allow up 2 channels to be used, 0 and 1 */
		0, /* assume any amount of incoming bandwidth */
		0 /* assume any amount of outgoing bandwidth */
	);

	if(platform_network.host == null)
	{
		error(false);
	}

	t_update_game* update_game = null;
	t_parse_packet* parse_packet = null;
	void* game_memory = null;
	HMODULE dll = null;
	s_platform_data platform_data = zero;

	{
		s_lin_arena all = zero;
		all.capacity = 10 * c_mb;
		all.memory = VirtualAlloc(null, all.capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		game_memory = la_get(&all, 1 * c_mb);
		game_network.read_arena = make_lin_arena_from_memory(1 * c_mb, la_get(&all, 1 * c_mb));
		game_network.write_arena = make_lin_arena_from_memory(1 * c_mb, la_get(&all, 1 * c_mb));
		platform_data.frame_arena = make_lin_arena_from_memory(5 * c_mb, la_get(&all, 5 * c_mb));
	}

	f64 time_passed = 0;
	while(true)
	{
		f64 start_of_frame_seconds = get_seconds();

		if(!update_game)
		{
			dll = LoadLibrary("build/server.dll");
			assert(dll);
			update_game = (t_update_game*)GetProcAddress(dll, "update_game");
			assert(update_game);
			parse_packet = (t_parse_packet*)GetProcAddress(dll, "parse_packet");
			assert(parse_packet);
		}

		enet_loop(&platform_network, &game_network, parse_packet);

		platform_data.time_passed = time_passed;
		update_game(platform_data, &game_network, game_memory);

		foreach_raw(packet_i, packet, game_network.out_packets)
		{
			ENetPacket* enet_packet = enet_packet_create(packet.data, packet.size, packet.flag);
			if(packet.broadcast)
			{
				enet_host_broadcast(platform_network.host, 0, enet_packet);
			}
			else
			{
				// @Note(tkap, 26/06/2023): Find peer from id
				b8 found = false;
				foreach_raw(peer_i, peer, platform_network.peers)
				{
					if(peer.id == packet.target)
					{
						enet_peer_send(peer.peer, 0, enet_packet);
						found = true;
						break;
					}
				}
				if(!found)
				{
					log("Tried to send a packet to non-existent peer %u. BIK PROBLEM OKAYEG", packet.target);
				}

			}
		}
		game_network.out_packets.count = 0;
		game_network.read_arena.used = 0;
		game_network.write_arena.used = 0;

		Sleep(2);

		time_passed = get_seconds() - start_of_frame_seconds;
	}

	return 0;
}

func void enet_loop(s_platform_network* platform_network, s_game_network* game_network, t_parse_packet* parse_packet)
{
	ENetEvent event;
	while(enet_host_service(platform_network->host, &event, 0) > 0)
	{
		switch(event.type)
		{
			case ENET_EVENT_TYPE_NONE:
			{
			} break;

			case ENET_EVENT_TYPE_CONNECT:
			{
				if(platform_network->peers.count >= c_max_peers) { break; }

				s_packet packet = zero;
				packet.size = sizeof(e_packet_connect);
				packet.data = (u8*)la_get(&game_network->read_arena, packet.size);
				packet.from = event.peer->connectID;
				u8* cursor = packet.data;
				int temp = e_packet_connect;
				buffer_write(&cursor, &temp, sizeof(temp));

				s_peer peer = zero;
				peer.id = event.peer->connectID;
				peer.peer = event.peer;
				platform_network->peers.add(peer);

				game_network->peers.add(peer.id);

				parse_packet(packet);

			} break;

			case ENET_EVENT_TYPE_DISCONNECT:
			{
				u32 disconnected_id = 0;
				foreach_raw(peer_i, peer, platform_network->peers)
				{
					if(peer.peer->connectID == 0)
					{
						assert(peer.id != 0);
						disconnected_id = peer.id;
						platform_network->peers.remove_and_shift(peer_i);
						game_network->peers.remove_and_shift(peer_i);
						peer_i -= 1;
						break;
					}
				}
				assert(disconnected_id != 0);

				s_packet packet = zero;
				packet.size = sizeof(e_packet_disconnect);
				packet.data = (u8*)la_get(&game_network->read_arena, packet.size);
				packet.from = disconnected_id;
				u8* cursor = packet.data;
				int temp = e_packet_disconnect;
				buffer_write(&cursor, &temp, sizeof(temp));
				parse_packet(packet);

			} break;

			case ENET_EVENT_TYPE_RECEIVE:
			{
				s_packet packet = zero;
				packet.size = (int)event.packet->dataLength;
				packet.data = (u8*)la_get(&game_network->read_arena, packet.size);
				packet.from = event.peer->connectID;
				memcpy(packet.data, event.packet->data, packet.size);
				parse_packet(packet);
				enet_packet_destroy(event.packet);
			} break;

			invalid_default_case;
		}
	}
}