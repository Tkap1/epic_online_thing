
global float spawn_timer[e_projectile_type_count];
global int current_level;
global float level_timer;

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
	e.dead[index] = false;
	e.time_lived[index] = 0;
	e.duration[index] = 0;
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


func int make_player(u32 player_id, b8 dead)
{
	int entity = make_entity();
	e.x[entity] = 100;
	e.y[entity] = 100;
	e.sx[entity] = 32;
	e.sy[entity] = 64;
	e.player_id[entity] = player_id;
	e.speed[entity] = 400;
	e.dead[entity] = dead;
	e.flags[entity][e_entity_flag_draw] = true;
	e.type[entity] = e_entity_type_player;

	#ifdef m_client
	if(player_id == my_id)
	{
		e.flags[entity][e_entity_flag_move] = true;
		e.flags[entity][e_entity_flag_input] = true;
		e.flags[entity][e_entity_flag_bounds_check] = true;
		e.flags[entity][e_entity_flag_gravity] = true;
	}
	#endif // m_client

	return entity;
}

func int make_projectile()
{
	int entity = make_entity();
	assert(entity != c_invalid_entity);
	e.type[entity] = e_entity_type_projectile;
	e.flags[entity][e_entity_flag_move] = true;
	e.flags[entity][e_entity_flag_draw_circle] = true;
	e.flags[entity][e_entity_flag_expire] = true;
	e.flags[entity][e_entity_flag_collide] = true;

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
					e.vel_y[entity] = randf_range(&rng, 400, 500);
					e.sx[entity] = randf_range(&rng, 48, 56);
					e.color[entity] = v4(0.9f, 0.1f, 0.1f, 1.0f);
				} break;

				case e_projectile_type_left_basic:
				{
					e.x[entity] = -100;
					e.y[entity] = randf_range(&rng, c_base_res.y * 0.6f, c_base_res.y);
					e.dir_x[entity] = 1;
					e.speed[entity] = randf_range(&rng, 400, 500);
					e.sx[entity] = randf_range(&rng, 38, 46);
					e.color[entity] = v4(0.1f, 0.9f, 0.1f, 1.0f);
				} break;

				case e_projectile_type_right_basic:
				{
					e.x[entity] = c_base_res.x + 100;
					e.y[entity] = randf_range(&rng, c_base_res.y * 0.6f, c_base_res.y);
					e.dir_x[entity] = -1;
					e.speed[entity] = randf_range(&rng, 400, 500);
					e.sx[entity] = randf_range(&rng, 38, 46);
					e.color[entity] = v4(0.1f, 0.9f, 0.1f, 1.0f);
				} break;

				invalid_default_case;
			}
		}
	}
}

func void init_levels()
{
	levels[0].spawn_delay[e_projectile_type_top_basic] = 0.4f;
	levels[1].spawn_delay[e_projectile_type_left_basic] = 0.5f;
	levels[2].spawn_delay[e_projectile_type_right_basic] = 0.4f;

	levels[3].spawn_delay[e_projectile_type_top_basic] = 0.5f;
	levels[3].spawn_delay[e_projectile_type_left_basic] = 0.5f;

	levels[4].spawn_delay[e_projectile_type_top_basic] = 0.4f;
	levels[4].spawn_delay[e_projectile_type_right_basic] = 0.5f;

	levels[5].spawn_delay[e_projectile_type_top_basic] = 0.1f;

	current_level = 0;
}

func void reset_level()
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
