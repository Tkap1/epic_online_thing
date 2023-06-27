#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "external/enet/enet.h"
#include "types.h"
#include "utils.h"
#include "memory.h"
#include "epic_time.h"
#include "shared_all.h"
#include "platform_shared_server.h"
#include "shared_client_server.h"
#include "win32_platform_server.h"
#include "platform_shared.h"

#include "memory.cpp"
#include "enet_shared_server.cpp"


int main(void)
{
	init_performance();

	if(enet_initialize() != 0)
	{
		error(false);
	}

	ENetAddress address = zero;
	address.host = ENET_HOST_ANY;
	address.port = c_internal_server_port;

	s_platform_network platform_network = zero;

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
	void *server_so = null;

	s_lin_arena all = zero;
	all.capacity = 10 * c_mb;
	all.memory = calloc(all.capacity, 1);
	void *game_memory = la_get(&all, 1 * c_mb);
	s_game_network game_network = zero;
	s_platform_data platform_data = zero;
	game_network.read_arena = make_lin_arena_from_memory(1 * c_mb, la_get(&all, 1*c_mb));
	game_network.write_arena = make_lin_arena_from_memory(1 * c_mb, la_get(&all, 1*c_mb));
	platform_data.frame_arena = make_lin_arena_from_memory(5 * c_mb, la_get(&all, 5*c_mb));

	f64 time_passed = 0;
	b8 need_so_reload = true;
	while(true)
	{
		f64 start_of_frame_seconds = get_seconds();

		// TODO: hot reload server dll
		if(need_so_reload)
		{
			int dl_flags = RTLD_NOW; // resolve function symbols immediately upon loading
			server_so = dlopen("./server.so", dl_flags);
			update_game = (t_update_game*)dlsym(server_so, "update_game");
			assert(update_game);
			parse_packet = (t_parse_packet*)dlsym(server_so, "parse_packet");
			assert(parse_packet);
			need_so_reload = false;
			platform_data.recompiled = true;
			update_game(platform_data, &game_network, game_memory, true);
		}

		enet_loop(&platform_network, &game_network, parse_packet);

		platform_data.time_passed = time_passed;
		update_game(platform_data, &game_network, game_memory, false);
		platform_data.recompiled = false;

		for(int packet_i = 0; packet_i < game_network.out_packets.count; ++packet_i)
		{
			s_packet packet = game_network.out_packets[packet_i];
			ENetPacket* enet_packet = enet_packet_create(packet.data, packet.size, packet.flag);
			if(packet.broadcast)
			{
				enet_host_broadcast(platform_network.host, 0, enet_packet);
			}
			else
			{
				// @Note(tkap, 26/06/2023): Find peer from id
				b8 found = false;
				for (int peer_i = 0; peer_i < platform_network.peers.count; ++peer_i)
				{
					s_peer peer = platform_network.peers[peer_i];
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

		struct timespec rq,rm;
		rq.tv_sec = 0;
		rq.tv_nsec = 20000000L; // 2ms
		nanosleep(&rq, &rm);

		time_passed = get_seconds() - start_of_frame_seconds;
	}

	return 0;
}
