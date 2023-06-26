

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