


func void move_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_move]) { continue; }

		e.x[ii] += e.dir_x[ii] * e.speed[ii] * delta;
		e.y[ii] += e.vel_y[ii] * delta;

		if(e.vel_y[ii] >= 0) { e.jumping[ii] = false; }
	}
}

func int make_entity()
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
	e.vel_y[index] = 0;
	e.jumps_done[index] = 0;
	e.player_id[index] = 0;
	e.jumping[index] = false;
}

func int find_player_by_id(u32 id)
{
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

func void bounds_check_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_bounds_check]) { continue; }

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


func int make_player(u32 player_id)
{
	int entity = make_entity();
	e.x[entity] = 100;
	e.y[entity] = 100;
	e.sx[entity] = 32;
	e.sy[entity] = 64;
	e.player_id[entity] = player_id;
	e.speed[entity] = 300;
	e.flags[entity][e_entity_flag_move] = true;
	e.flags[entity][e_entity_flag_draw] = true;
	e.flags[entity][e_entity_flag_bounds_check] = true;
	e.flags[entity][e_entity_flag_gravity] = true;

	#ifdef m_client
	if(player_id == my_id)
	{
		e.flags[entity][e_entity_flag_input] = true;
	}
	#endif // m_client

	return entity;
}
