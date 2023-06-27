

func void enet_loop(ENetHost* client, int timeout, s_game_network* game_network, t_parse_packet* parse_packet)
{
	ENetEvent event;
	while(enet_host_service(client, &event, timeout) > 0)
	{
		switch(event.type)
		{
			case ENET_EVENT_TYPE_NONE:
			{
			} break;

			case ENET_EVENT_TYPE_CONNECT:
			{
				s_packet packet = zero;
				packet.size = sizeof(e_packet_connect);
				packet.data = (u8*)la_get(&game_network->read_arena, packet.size);
				u8* cursor = packet.data;
				int temp = e_packet_connect;
				buffer_write(&cursor, &temp, sizeof(temp));
				parse_packet(packet);
			} break;

			case ENET_EVENT_TYPE_DISCONNECT:
			{
				s_packet packet = zero;
				packet.size = sizeof(e_packet_disconnect);
				packet.data = (u8*)la_get(&game_network->read_arena, packet.size);
				u8* cursor = packet.data;
				int temp = e_packet_disconnect;
				buffer_write(&cursor, &temp, sizeof(temp));
				parse_packet(packet);
				return;
			} break;

			case ENET_EVENT_TYPE_RECEIVE:
			{
				s_packet packet = zero;
				packet.size = (int)event.packet->dataLength;
				packet.data = (u8*)la_get(&game_network->read_arena, packet.size);;
				memcpy(packet.data, event.packet->data, packet.size);
				parse_packet(packet);
				enet_packet_destroy(event.packet);
			} break;

			invalid_default_case;
		}
	}
}

func void connect_to_server(s_platform_network* platform_network, s_game_network* game_network)
{
	if(enet_initialize() != 0)
	{
		error(false);
	}

	platform_network->client = enet_host_create(
		null /* create a client host */,
		1, /* only allow 1 outgoing connection */
		2, /* allow up 2 channels to be used, 0 and 1 */
		0, /* assume any amount of incoming bandwidth */
		0 /* assume any amount of outgoing bandwidth */
	);

	if(platform_network->client == null)
	{
		error(false);
	}

	ENetAddress address = zero;
	enet_address_set_host(&address, game_network->ip.data);
	address.port = (u16)game_network->port;

	platform_network->server = enet_host_connect(platform_network->client, &address, 2, 0);
	if(platform_network->server == null)
	{
		error(false);
	}

	game_network->connected = true;

}