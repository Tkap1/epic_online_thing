#define m_server 1

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include "external/enet/enet.h"
#include <stdio.h>

#include "types.h"
#include "utils.h"
#include "memory.h"
#include "epic_time.h"
#include "shared_all.h"
#include "platform_shared_server.h"
#include "shared_client_server.h"
#include "win32_platform_server.h"
#include "platform_shared.h"
#include "win32_reload_dll.h"

#include "memory.cpp"
#include "enet_shared_server.cpp"


int main(int argc, char** argv)
{
	unreferenced(argc);
	unreferenced(argv);

	init_performance();

	if(enet_initialize() != 0)
	{
		error(false);
	}

	ENetAddress address = zero;
	address.host = ENET_HOST_ANY;
	address.port = c_internal_server_port;

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
	s_lin_arena frame_arena = zero;
	s_lin_arena permanent_arena = zero;

	{
		s_lin_arena all = zero;
		all.capacity = 20 * c_mb;

		// @Note(tkap, 26/06/2023): We expect this memory to be zero'd
		all.memory = VirtualAlloc(null, all.capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

		game_memory = la_get(&all, c_game_memory);
		game_network.read_arena = make_lin_arena_from_memory(1 * c_mb, la_get(&all, 1 * c_mb));
		game_network.write_arena = make_lin_arena_from_memory(1 * c_mb, la_get(&all, 1 * c_mb));
		frame_arena = make_lin_arena_from_memory(5 * c_mb, la_get(&all, 5 * c_mb));
		platform_data.frame_arena = &frame_arena;

		permanent_arena = make_lin_arena_from_memory(5 * c_mb, la_get(&all, 5 * c_mb));
		platform_data.permanent_arena = &permanent_arena;
	}

	f64 time_passed = 0;
	while(true)
	{
		f64 start_of_frame_seconds = get_seconds();

		if(need_to_reload_dll("build/server.dll"))
		{
			if(dll) { unload_dll(dll); }

			for(int i = 0; i < 100; i++)
			{
				if(CopyFile("build/server.dll", "server.dll", false)) { break; }
				assert(i != 99);
				Sleep(10);
			}
			dll = load_dll("server.dll");
			update_game = (t_update_game*)GetProcAddress(dll, "update_game");
			assert(update_game);
			parse_packet = (t_parse_packet*)GetProcAddress(dll, "parse_packet");
			assert(parse_packet);
			printf("Reloaded DLL!\n");
			platform_data.recompiled = true;
			update_game(platform_data, &game_network, game_memory, true);
		}


		enet_loop(&platform_network, &game_network, parse_packet);

		platform_data.time_passed = time_passed;
		update_game(platform_data, &game_network, game_memory, false);
		platform_data.recompiled = false;

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
