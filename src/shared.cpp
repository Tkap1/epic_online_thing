
global float spawn_timer[e_projectile_type_count];
global int current_level;
global float level_timer;

func void player_movement_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_player_movement]) { continue; }

		e.x[ii] += e.dir_x[ii] * e.speed[ii] * delta;
		e.y[ii] += e.vel_y[ii] * delta;

		if(e.vel_y[ii] >= 0) { e.jumping[ii] = false; }
	}
}

func void move_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_move]) { continue; }

		e.x[ii] += e.dir_x[ii] * e.speed[ii] * delta;
		e.y[ii] += e.dir_y[ii] * e.speed[ii] * delta;
	}
}

func void physics_movement_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_physics_movement]) { continue; }

		e.x[ii] += e.vel_x[ii] * delta;
		e.y[ii] += e.vel_y[ii] * delta;
	}
}

func int make_entity(void)
{
	for(int i = 0; i < c_max_entities; i++)
	{
		if(!e.active[i])
		{
			zero_entity(i);
			e.active[i] = true;
			e.id[i] = ++e.next_id;
			return i;
		}
	}
	assert(false);
	return c_invalid_entity;
}

func void zero_entity(int index)
{
	assert(index < c_max_entities);
	memset(e.flags[index], 0, sizeof(e.flags[index]));
	e.x[index] = 0;
	e.y[index] = 0;
	e.sx[index] = 0;
	e.sy[index] = 0;
	e.speed[index] = 0;
	e.dir_x[index] = 0;
	e.dir_y[index] = 0;
	e.vel_x[index] = 0;
	e.vel_y[index] = 0;
	e.jumps_done[index] = 0;
	e.player_id[index] = 0;
	e.jumping[index] = false;
	e.dead[index] = false;
	e.time_lived[index] = 0;
	e.duration[index] = 0;
	e.spawn_timer[index] = 0;
	e.name[index] = zero;
	e.drawn_last_render[index] = false;
}

func int find_player_by_id(u32 id)
{
	if(id == 0) { return c_invalid_entity; }

	for(int i = 0; i < c_max_entities; i++)
	{
		if(!e.active[i]) { continue; }
		if(e.player_id[i] == id) { return i; }
	}
	return c_invalid_entity;
}

func void gravity_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_gravity]) { continue; }

		e.vel_y[ii] += c_gravity * delta;
	}
}

func void player_bounds_check_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_player_bounds_check]) { continue; }

		float half_x = e.sx[ii] * 0.5f;
		float half_y = e.sy[ii] * 0.5f;
		if(e.x[ii] - half_x < 0) { e.x[ii] = half_x; }
		if(e.x[ii] + half_x > c_base_res.x) { e.x[ii] = c_base_res.x - half_x; }
		if(e.y[ii] - half_y < 0) { e.y[ii] = half_y; }
		if(e.y[ii] + half_y > c_base_res.y)
		{
			e.jumping[ii] = false;
			e.jumps_done[ii] = 0;
			e.vel_y[ii] = 0;
			e.y[ii] = c_base_res.y - half_y;
		}
	}
}

func void projectile_bounds_check_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_projectile_bounds_check]) { continue; }

		float radius = e.sx[ii];
		if(
			e.x[ii] + radius < -c_projectile_out_of_bounds_offset ||
			e.y[ii] + radius < -c_projectile_out_of_bounds_offset ||
			e.x[ii] - radius > c_base_res.x + c_projectile_out_of_bounds_offset ||
			e.y[ii] - radius > c_base_res.y + c_projectile_out_of_bounds_offset
		)
		{
			e.active[ii] = false;
		}
	}
}


func int make_player(u32 player_id, b8 dead, s_v4 color)
{
	int entity = make_entity();
	e.x[entity] = c_spawn_pos.x;
	e.y[entity] = c_spawn_pos.y;
	handle_instant_movement(entity);
	e.sx[entity] = 32;
	e.sy[entity] = 64;
	e.player_id[entity] = player_id;
	e.speed[entity] = 400;
	e.dead[entity] = dead;
	e.flags[entity][e_entity_flag_draw] = true;
	e.type[entity] = e_entity_type_player;
	e.color[entity] = color;

	#ifdef m_client
	if(player_id == my_id)
	{
		e.flags[entity][e_entity_flag_player_movement] = true;
		e.flags[entity][e_entity_flag_input] = true;
		e.flags[entity][e_entity_flag_player_bounds_check] = true;
		e.flags[entity][e_entity_flag_gravity] = true;
	}
	#endif // m_client


	return entity;
}

func int make_projectile(void)
{
	int entity = make_entity();
	assert(entity != c_invalid_entity);
	e.type[entity] = e_entity_type_projectile;
	e.flags[entity][e_entity_flag_move] = true;
	e.flags[entity][e_entity_flag_draw_circle] = true;
	e.flags[entity][e_entity_flag_expire] = true;
	e.flags[entity][e_entity_flag_collide] = true;
	e.flags[entity][e_entity_flag_projectile_bounds_check] = true;

	return entity;
}


func void spawn_system(s_level level)
{

	for(int proj_i = 0; proj_i < e_projectile_type_count; proj_i++)
	{
		if(level.spawn_delay[proj_i] <= 0) { continue; }
		spawn_timer[proj_i] += delta;
		while(spawn_timer[proj_i] >= level.spawn_delay[proj_i])
		{
			spawn_timer[proj_i] -= level.spawn_delay[proj_i];
			int entity = make_projectile();

			switch(proj_i)
			{
				case e_projectile_type_top_basic:
				{
					e.x[entity] = randf32(&rng) * c_base_res.x;
					e.y[entity] = -c_projectile_spawn_offset;
					e.dir_y[entity] = 1;
					e.speed[entity] = randf_range(&rng, 400, 500);
					e.sx[entity] = randf_range(&rng, 48, 56);
					e.color[entity] = v4(0.9f, 0.1f, 0.1f, 1.0f);

					e.speed[entity] *= level.speed_multiplier[proj_i];
					handle_instant_movement(entity);
				} break;

				case e_projectile_type_left_basic:
				{
					make_side_projectile(entity, -c_projectile_spawn_offset, 1);
					e.speed[entity] *= level.speed_multiplier[proj_i];
					handle_instant_movement(entity);
				} break;

				case e_projectile_type_right_basic:
				{
					make_side_projectile(entity, c_base_res.x + c_projectile_spawn_offset, -1);
					e.speed[entity] *= level.speed_multiplier[proj_i];
					handle_instant_movement(entity);
				} break;

				case e_projectile_type_diagonal_left:
				{
					make_diagonal_top_projectile(entity, 0, c_base_res.x);
					e.speed[entity] *= level.speed_multiplier[proj_i];
					handle_instant_movement(entity);
				} break;

				case e_projectile_type_diagonal_right:
				{
					make_diagonal_top_projectile(entity, c_base_res.x, 0);
					e.speed[entity] *= level.speed_multiplier[proj_i];
					handle_instant_movement(entity);
				} break;

				case e_projectile_type_diagonal_bottom_left:
				{
					float angle = pi * -0.25f;
					make_diagonal_bottom_projectile(entity, 0.0f, angle);
					e.speed[entity] *= level.speed_multiplier[proj_i];
					handle_instant_movement(entity);
				} break;

				case e_projectile_type_diagonal_bottom_right:
				{
					float angle = pi * -0.75f;
					make_diagonal_bottom_projectile(entity, c_base_res.x, angle);
					e.speed[entity] *= level.speed_multiplier[proj_i];
					handle_instant_movement(entity);
				} break;

				case e_projectile_type_cross:
				{
					float x;
					float y;
					float size = randf_range(&rng, 15, 44);
					float speed = randf_range(&rng, 125, 255);

					if ((randu(&rng) & 1) == 0)
					{
						x = randf_range(&rng, 0, c_base_res.x / 4);
					}
					else
					{
						x = randf_range(&rng, c_base_res.x - c_base_res.x / 4, c_base_res.x);
						speed *= randf_range(&rng, 1.5f, 2.5f);
					}

					if ((randu(&rng) & 1) == 0)
					{
						y = randf_range(&rng, c_base_res.y - c_base_res.y / 8, c_base_res.y);
					}
					else
					{
						y = randf_range(&rng, 0, c_base_res.y);
					}

					s_v4 col = v4(randf_range(&rng, 0, 1.0f), randf_range(&rng, 0, 1.0f), randf_range(&rng, 0, 1.0f), 1.0f);

					e.x[entity] = x;
					e.y[entity] = y;
					e.sx[entity] = size;
					e.speed[entity] = speed * level.speed_multiplier[proj_i];;
					e.dir_x[entity] = -1.0f;
					e.dir_y[entity] = 0.0f;
					e.color[entity] = col;
					handle_instant_movement(entity);

					entity = make_projectile();
					e.x[entity] = x;
					e.y[entity] = y;
					e.sx[entity] = size;
					e.speed[entity] = speed * level.speed_multiplier[proj_i];;
					e.dir_x[entity] = 1.0f;
					e.dir_y[entity] = 0.0f;
					e.color[entity] = col;
					handle_instant_movement(entity);

					entity = make_projectile();
					e.x[entity] = x;
					e.y[entity] = y;
					e.sx[entity] = size;
					e.speed[entity] = 133 * level.speed_multiplier[proj_i];;
					e.dir_x[entity] = 0.0f;
					e.dir_y[entity] = -1.0f;
					e.color[entity] = col;
					handle_instant_movement(entity);

					entity = make_projectile();
					e.x[entity] = x;
					e.y[entity] = y;
					e.sx[entity] = size;
					e.speed[entity] = 133 * level.speed_multiplier[proj_i];;
					e.dir_x[entity] = 0.0f;
					e.dir_y[entity] = 1.0f;
					e.color[entity] = col;
					handle_instant_movement(entity);
				} break;

				case e_projectile_type_spawner:
				{
					e.x[entity] = -c_projectile_spawn_offset;
					e.y[entity] = c_base_res.y * 0.5f;
					e.dir_x[entity] = 1;
					e.speed[entity] = 300;
					e.sx[entity] = 50;
					e.color[entity] = v4(0.1f, 0.1f, 0.9f, 1.0f);
					e.spawn_delay[entity] = 1.0f;
					e.flags[entity][e_entity_flag_projectile_spawner] = true;

					e.speed[entity] *= level.speed_multiplier[proj_i];
					handle_instant_movement(entity);
				} break;

				invalid_default_case;
			}
		}
	}
}

func void init_levels(void)
{
	#define speed(val) (1000.0f / val)

	for(int level_i = 0; level_i < c_max_levels; level_i++)
	{
		for(int proj_i = 0; proj_i < e_projectile_type_count; proj_i++)
		{
			levels[level_i].speed_multiplier[proj_i] = 1;
		}
	}

	levels[level_count].spawn_delay[e_projectile_type_top_basic] = speed(3500);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_left_basic] = speed(2000);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_diagonal_right] = speed(3000);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_right_basic] = speed(2500);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_diagonal_left] = speed(2800);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_diagonal_bottom_right] = speed(2800);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_spawner] = speed(1000);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_diagonal_bottom_left] = speed(3300);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_left_basic] = speed(1000);
	levels[level_count].spawn_delay[e_projectile_type_right_basic] = speed(1000);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_cross] = speed(2777);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_top_basic] = speed(3000);
	levels[level_count].spawn_delay[e_projectile_type_left_basic] = speed(2500);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_diagonal_left] = speed(3500);
	levels[level_count].spawn_delay[e_projectile_type_diagonal_bottom_right] = speed(2500);
	level_count++;

	levels[level_count].spawn_delay[e_projectile_type_top_basic] = speed(2000);
	levels[level_count].spawn_delay[e_projectile_type_spawner] = speed(1500);
	level_count++;

	current_level = 0;
	#undef speed
}

func void reset_level(void)
{
	level_timer = 0;
	memset(spawn_timer, 0, sizeof(spawn_timer));

	// @Note(tkap, 22/06/2023): Remove projectiles
	for(int i = 0; i < c_max_entities; i++)
	{
		if(!e.active[i]) { continue; }
		if(e.type[i] == e_entity_type_projectile)
		{
			e.active[i] = false;
		}
	}

	// @Note(tkap, 23/06/2023): Place every player at the spawn position
	for(int i = 0; i < c_max_entities; i++)
	{
		if(!e.active[i]) { continue; }
		if(!e.player_id[i]) { continue; }

		e.x[i] = c_spawn_pos.x;
		e.y[i] = c_spawn_pos.y;
		e.jumping[i] = false;
		e.vel_y[i] = 0;
		e.jumps_done[i] = 1;
		handle_instant_movement(i);
	}
}

func void expire_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_expire]) { continue; }

		e.time_lived[ii] += delta;
		if(e.time_lived[ii] >= e.duration[ii])
		{
			e.active[ii] = false;
		}
	}
}

func void make_diagonal_bottom_projectile(int entity, float x, float angle)
{
	e.x[entity] = x;
	e.y[entity] = c_base_res.y;
	s_v2 vel = v2_mul(v2_from_angle(angle), randf_range(&rng, 200, 2000));
	e.vel_x[entity] = vel.x;
	e.vel_y[entity] = vel.y;
	e.sx[entity] = randf_range(&rng, 38, 46);
	e.color[entity] = v4(0.1f, 0.4f, 0.4f, 1.0f);
	e.flags[entity][e_entity_flag_physics_movement] = true;
	e.flags[entity][e_entity_flag_move] = false;
	e.flags[entity][e_entity_flag_gravity] = true;
}

func void make_diagonal_top_projectile(int entity, float x, float opposite_x)
{
	s_v2 pos = v2(x, 0.0f);
	e.x[entity] = pos.x;
	e.y[entity] = pos.y;
	s_v2 a = v2_sub(v2(opposite_x, c_base_res.y * 0.7f), pos);
	s_v2 b = v2_sub(v2(x, c_base_res.y), pos);
	float angle = randf_range(&rng, v2_angle(a), v2_angle(b));
	s_v2 dir = v2_from_angle(angle);
	e.dir_x[entity] = dir.x;
	e.dir_y[entity] = dir.y;
	e.speed[entity] = randf_range(&rng, 400, 500);
	e.sx[entity] = randf_range(&rng, 38, 46);
	e.color[entity] = v4(0.9f, 0.9f, 0.1f, 1.0f);
}

func void projectile_spawner_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_projectile_spawner]) { continue; }

		e.spawn_timer[ii] += delta;
		while(e.spawn_timer[ii] >= e.spawn_delay[ii])
		{
			e.spawn_timer[ii] -= e.spawn_delay[ii];
			for(int proj_i = 0; proj_i < 2; proj_i++)
			{
				int entity = make_projectile();
				float angle = randf_range(&rng, pi * 0.3f, pi * 0.7f);
				e.x[entity] = e.x[ii];
				e.y[entity] = e.y[ii];
				e.sx[entity] = e.sx[ii] * 0.5f;
				e.speed[entity] = e.speed[ii] * 0.5f;
				e.dir_x[entity] = cosf(angle);
				e.dir_y[entity] = sinf(angle);
				e.color[entity] = v41f(0.5f);
				handle_instant_movement(entity);
			}
		}
	}
}

func void make_side_projectile(int entity, float x, float x_dir)
{
	e.x[entity] = x;
	e.y[entity] = randf_range(&rng, c_base_res.y * 0.6f, c_base_res.y);
	e.dir_x[entity] = x_dir;
	e.speed[entity] = randf_range(&rng, 400, 500);
	e.sx[entity] = randf_range(&rng, 38, 46);
	e.color[entity] = v4(0.1f, 0.9f, 0.1f, 1.0f);
}

func void send_packet_(ENetPeer* peer, e_packet packet_id, void* data, size_t size, int flag)
{
	assert(flag == 0 || flag == ENET_PACKET_FLAG_RELIABLE);
	assert(size <= 1024 - sizeof(packet_id));

	u8 packet_data[1024];
	u8* cursor = packet_data;
	buffer_write(&cursor, &packet_id, sizeof(packet_id));
	buffer_write(&cursor, data, size);
	ENetPacket* packet = enet_packet_create(packet_data, cursor - packet_data, flag);
	enet_peer_send(peer, 0, packet);
}

func void broadcast_packet_(ENetHost* in_host, e_packet packet_id, void* data, size_t size, int flag)
{
	assert(flag == 0 || flag == ENET_PACKET_FLAG_RELIABLE);
	assert(size <= 1024 - sizeof(packet_id));

	u8 packet_data[1024];
	u8* cursor = packet_data;
	buffer_write(&cursor, &packet_id, sizeof(packet_id));
	buffer_write(&cursor, data, size);
	ENetPacket* packet = enet_packet_create(packet_data, cursor - packet_data, flag);
	enet_host_broadcast(in_host, 0, packet);
}

func s_name str_to_name(char* str)
{
	s_name result = zero;
	result.len = (int)strlen(str);
	assert(result.len < max_player_name_length);
	memcpy(result.data, str, result.len + 1);
	return result;
}

func void send_simple_packet(ENetPeer* peer, e_packet packet_id, int flag)
{
	assert(flag == 0 || flag == ENET_PACKET_FLAG_RELIABLE);

	u8 packet_data[4];
	u8* cursor = packet_data;
	buffer_write(&cursor, &packet_id, sizeof(packet_id));
	ENetPacket* packet = enet_packet_create(packet_data, sizeof(packet_id), flag);
	enet_peer_send(peer, 0, packet);
}

func void broadcast_simple_packet(ENetHost* in_host, e_packet packet_id, int flag)
{
	assert(flag == 0 || flag == ENET_PACKET_FLAG_RELIABLE);

	u8 packet_data[4];
	u8* cursor = packet_data;
	buffer_write(&cursor, &packet_id, sizeof(packet_id));
	ENetPacket* packet = enet_packet_create(packet_data, sizeof(packet_id), flag);
	enet_host_broadcast(in_host, 0, packet);
}