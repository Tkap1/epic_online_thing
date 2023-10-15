
global float spawn_timer[c_max_spawns_per_level];
global float level_timer;
global s_level levels[c_max_levels];

func void player_movement_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_player_movement]) { continue; }

		if(game->e.dashing[ii])
		{
			game->e.x[ii] += game->e.dash_dir[ii] * c_dash_speed * delta;
			game->e.dash_timer[ii] += delta;
			game->e.vel_y[ii] = 0;

			if(game->e.dash_timer[ii] >= c_dash_duration)
			{
				cancel_dash(ii);
			}
		}
		else
		{
			game->e.x[ii] += game->e.dir_x[ii] * game->e.modified_speed[ii] * delta;
			game->e.y[ii] += game->e.vel_y[ii] * delta;

			if(game->e.vel_y[ii] >= 0) { game->e.jumping[ii] = false; }
		}
	}
}

func void move_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_move]) { continue; }

		game->e.x[ii] += game->e.dir_x[ii] * game->e.modified_speed[ii] * delta;
		game->e.y[ii] += game->e.dir_y[ii] * game->e.modified_speed[ii] * delta;
	}
}

func void physics_movement_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_physics_movement]) { continue; }

		game->e.x[ii] += game->e.vel_x[ii] * delta;
		game->e.y[ii] += game->e.vel_y[ii] * delta;
	}
}

func int make_entity(void)
{
	for(int i = 0; i < c_max_entities; i++)
	{
		if(!game->e.active[i])
		{
			zero_entity(i);
			game->e.active[i] = true;
			game->e.id[i] = ++game->e.next_id;
			return i;
		}
	}
	assert(false);
	return c_invalid_entity;
}

func void zero_entity(int index)
{
	assert(index < c_max_entities);
	memset(game->e.flags[index], 0, sizeof(game->e.flags[index]));
	game->e.x[index] = 0;
	game->e.y[index] = 0;
	set_size(index, 0, 0);
	set_speed(index, 0);
	game->e.speed_curve[index] = zero;
	game->e.size_curve[index] = zero;
	game->e.dir_x[index] = 0;
	game->e.dir_y[index] = 0;
	game->e.vel_x[index] = 0;
	game->e.vel_y[index] = 0;
	game->e.jumps_done[index] = 0;
	game->e.player_id[index] = 0;
	game->e.jumping[index] = false;
	game->e.on_ground[index] = false;
	game->e.dead[index] = false;
	game->e.time_lived[index] = 0;
	game->e.duration[index] = 0;
	game->e.particle_spawner[index].type = e_particle_spawner_default;
	game->e.particle_spawner[index].spawn_timer = 0;
	game->e.particle_spawner[index].spawn_delay = 0;
	game->e.name[index] = zero;
	game->e.drawn_last_render[index] = false;
	game->e.best_time_on_level[index] = 0;
	game->e.trail_timer[index] = 0;

	game->e.dash_timer[index] = 0;
	game->e.dashing[index] = false;
}

func int find_player_by_id(u32 id)
{
	if(id == 0) { return c_invalid_entity; }

	for(int i = 0; i < c_max_entities; i++)
	{
		if(!game->e.active[i]) { continue; }
		if(game->e.player_id[i] == id) { return i; }
	}
	return c_invalid_entity;
}

func void gravity_system(int start, int count)
{
	float gravity_multiplier = levels[game->current_level].gravity_multiplier;

	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_gravity]) { continue; }

		game->e.vel_y[ii] += c_gravity * gravity_multiplier * delta;
	}
}

func void player_bounds_check_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_player_bounds_check]) { continue; }

		float half_x = game->e.modified_sx[ii] * 0.5f;
		float half_y = game->e.modified_sy[ii] * 0.5f;
		if(game->e.x[ii] - half_x < 0) { game->e.x[ii] = half_x; }
		if(game->e.x[ii] + half_x > c_base_res.x) { game->e.x[ii] = c_base_res.x - half_x; }
		if(game->e.y[ii] - half_y < 0) { game->e.y[ii] = half_y; }
		if(game->e.y[ii] + half_y > c_base_res.y)
		{
			game->e.can_dash[ii] = true;
			game->e.jumping[ii] = false;
			game->e.on_ground[ii] = true;
			game->e.jumps_done[ii] = 0;
			game->e.vel_y[ii] = 0;
			game->e.y[ii] = c_base_res.y - half_y;
		}
	}
}

func void projectile_bounds_check_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_projectile_bounds_check]) { continue; }

		float radius = game->e.modified_sx[ii];
		if(
			game->e.x[ii] + radius < -game->e.out_of_bounds_offset[ii] ||
			game->e.y[ii] + radius < -game->e.out_of_bounds_offset[ii] ||
			game->e.x[ii] - radius > c_base_res.x + game->e.out_of_bounds_offset[ii] ||
			game->e.y[ii] - radius > c_base_res.y + game->e.out_of_bounds_offset[ii]
		)
		{
			game->e.active[ii] = false;
		}
	}
}


func int make_player(u32 player_id, b8 dead, s_v4 color)
{
	s_level level = levels[game->current_level];

	int entity = make_entity();
	game->e.x[entity] = level.spawn_pos.x;
	game->e.y[entity] = level.spawn_pos.y;
	handle_instant_movement(entity);
	set_size(entity, 32, 64);
	handle_instant_resize(entity);
	game->e.player_id[entity] = player_id;
	set_speed(entity, 400);
	game->e.dead[entity] = dead;
	game->e.flags[entity][e_entity_flag_draw] = true;
	game->e.flags[entity][e_entity_flag_increase_time_lived] = true;
	game->e.type[entity] = e_entity_type_player;
	game->e.color[entity] = color;
	game->e.can_dash[entity] = true;
	game->e.dash_dir[entity] = 1;

	#ifdef m_client
	if(player_id == game->my_id)
	{
		game->e.flags[entity][e_entity_flag_player_movement] = true;
		game->e.flags[entity][e_entity_flag_input] = true;
		game->e.flags[entity][e_entity_flag_player_bounds_check] = true;
		game->e.flags[entity][e_entity_flag_gravity] = true;
	}
	#endif // m_client


	return entity;
}

func int make_projectile()
{
	int entity = make_entity();
	assert(entity != c_invalid_entity);
	game->e.type[entity] = e_entity_type_projectile;
	game->e.flags[entity][e_entity_flag_move] = true;
	game->e.flags[entity][e_entity_flag_draw_circle] = true;
	game->e.flags[entity][e_entity_flag_expire] = true;
	game->e.flags[entity][e_entity_flag_collide] = true;
	game->e.flags[entity][e_entity_flag_projectile_bounds_check] = true;
	game->e.flags[entity][e_entity_flag_increase_time_lived] = true;
	game->e.flags[entity][e_entity_flag_modify_speed] = true;
	game->e.flags[entity][e_entity_flag_modify_size] = true;


	return entity;
}


func void spawn_system(s_level level)
{
	for(int spawn_i = 0; spawn_i < level.spawn_data.count; spawn_i++)
	{
		s_projectile_spawn_data data = level.spawn_data[spawn_i];
		spawn_timer[spawn_i] += delta;
		while(spawn_timer[spawn_i] >= data.delay && spawn_timer[spawn_i] >= data.frequency)
		{
			spawn_timer[spawn_i] -= data.frequency;

			// MARK: here
			int entity = make_projectile();
			game->e.x[entity] = randf_range(&game->rng, data.x[0], data.x[1]);
			game->e.y[entity] = randf_range(&game->rng, data.y[0], data.y[1]);
			float angle = randf_range(&game->rng, data.angle[0], data.angle[1]);
			s_v2 dir = v2_from_angle(angle * tau);
			game->e.dir_x[entity] = dir.x;
			game->e.dir_y[entity] = dir.y;
			set_speed(entity, randf_range(&game->rng, data.speed[0], data.speed[1]));
			set_size(entity, randf_range(&game->rng, data.size[0], data.size[1]), 0);

			if(data.hsv_colour)
			{
				game->e.color[entity] = v4(hsv_to_rgb(v3(
					data.rainbow_hue ? level_timer * 0.6f : randf_range(&game->rng, data.r[0], data.r[1]),
					randf_range(&game->rng, data.g[0], data.g[1]),
					randf_range(&game->rng, data.b[0], data.b[1])
					)),
					randf_range(&game->rng, data.a[0], data.a[1])
				);
			}
			else
			{
				game->e.color[entity] = v4(
					randf_range(&game->rng, data.r[0], data.r[1]),
					randf_range(&game->rng, data.g[0], data.g[1]),
					randf_range(&game->rng, data.b[0], data.b[1]),
					randf_range(&game->rng, data.a[0], data.a[1])
				);
			}

			if(data.on_spawn != e_on_spawn_invalid)
			{
				on_spawn(entity, data);
			}

			apply_projectile_modifiers(entity, data);
			handle_instant_movement(entity);
			handle_instant_resize(entity);
		}
	}
}

func void set_level_name(const char*name)
{
	assert(strlen(name) < c_max_level_name);
	strcpy(levels[game->level_count].name, name);
}

func void init_levels(void)
{
	game->level_count = 0;
	memset(levels, 0, sizeof(levels));

	s_v2 default_spawn_position = v2(c_base_res.x * 0.5f, c_base_res.y * 0.5f);

	for(int level_i = 0; level_i < c_max_levels; level_i++)
	{
		levels[level_i].spawn_pos = default_spawn_position;
		levels[level_i].gravity_multiplier = 1.f;
		levels[level_i].duration = c_level_duration;
		levels[level_i].background = e_background_default;
	}

	// @Note(tkap, 01/07/2023): Just for convenience, so we can just say "data = ..." instead of "s_projectile_spawn_data data = ..."
	s_projectile_spawn_data data = zero;

	set_level_name("Beginnings");
	levels[game->level_count].spawn_data.add(make_basic_top_projectile(4000, e_side_top));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Green Winds");
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(2000, e_side_left));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Solar Rays");
	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(3000, e_side_right));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Well Placed Landings");
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(1000, e_side_right));
	levels[game->level_count].spawn_data.add(make_ground_shot_projectile(3500));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Boots on Ground!");
	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(2800, e_side_left));
	levels[game->level_count].spawn_data.add(make_air_shot_projectile(10000));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Water Sport");
	levels[game->level_count].spawn_data.add(make_bottom_diagonal_projectile(2800, e_side_right));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Bombings");
	levels[game->level_count].spawn_data.add(make_spawner_projectile(1000, e_side_left));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Spinkler");
	levels[game->level_count].spawn_data.add(make_bottom_diagonal_projectile(3300, e_side_left));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Road Crossing");
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(1000, e_side_left));
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(1000, e_side_right));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Fireworks!");
	levels[game->level_count].spawn_data.add(make_cross_projectile(2777));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Crossroads");
	levels[game->level_count].spawn_data.add(make_basic_top_projectile(3000, e_side_top));
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(2500, e_side_left));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Sunshine and Rain");
	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(3500, e_side_left));
	levels[game->level_count].spawn_data.add(make_bottom_diagonal_projectile(2500, e_side_right));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Air Raid");
	levels[game->level_count].spawn_data.add(make_basic_top_projectile(2000, e_side_top));
	levels[game->level_count].spawn_data.add(make_spawner_projectile(1500, e_side_left));
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add(make_basic_side_projectile(2200, e_side_right));
	levels[game->level_count].spawn_data.add(make_bottom_diagonal_projectile(1000, e_side_left));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Sunburn");
	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(4000, e_side_left));
	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(4000, e_side_right));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("3-Way");
	levels[game->level_count].spawn_data.add(make_basic_top_projectile(4000, e_side_top));
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(1200, e_side_left));
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(1200, e_side_right));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Shower");
	levels[game->level_count].spawn_data.add(make_bottom_diagonal_projectile(2200, e_side_left));
	levels[game->level_count].spawn_data.add(make_bottom_diagonal_projectile(2200, e_side_right));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Red Suns");
	levels[game->level_count].spawn_data.add(make_corner_shot_projectile(3333));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("WTF");
	levels[game->level_count].spawn_data.add(make_shockwave_projectile(2777));
	game->level_count++;
	// -----------------------------------------------------------------------------

	// @Note(tkap, 25/06/2023): Maze
	set_level_name("Blood Rain Maze");
	data = make_basic_top_projectile(25000, e_side_top);
	data.speed[0] *= 0.33f;
	data.speed[1] *= 0.33f;
	data.size[0] *= 0.25f;
	data.size[1] *= 0.25f;
	data.speed_curve = {
		.start_seconds = {0},
		.end_seconds = {1},
		.multiplier = {5},
	},
	levels[game->level_count].spawn_data.add(data);
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Impending Doom");
	data = make_basic_side_projectile(2000, e_side_left);
	data.speed[0] *= 1.5f;
	data.speed[1] *= 1.5f;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(1400, e_side_right);
	data.speed[0] *= 0.25f;
	data.speed[1] *= 0.25f;
	levels[game->level_count].spawn_data.add(data);
	game->level_count++;
	// -----------------------------------------------------------------------------

	// @Note(tkap, 26/06/2023): Can't be on bottom, infinite jump
	set_level_name("Fly!");
	levels[game->level_count].infinite_jumps = true;
	data = make_basic_side_projectile(3000, e_side_left);
	data.speed[0] *= 2.0f;
	data.speed[1] *= 2.0f;
	data.y[0] = c_base_res.y * 0.96f;
	data.y[1] = c_base_res.y * 0.96f;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(3000, e_side_right);
	data.speed[0] *= 2.0f;
	data.speed[1] *= 2.0f;
	data.y[0] = c_base_res.y * 0.96f;
	data.y[1] = c_base_res.y * 0.96f;
	levels[game->level_count].spawn_data.add(data);

	levels[game->level_count].spawn_data.add(make_basic_side_projectile(1000, e_side_left));
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(1000, e_side_right));

	data = make_basic_top_projectile(8000, e_side_top);
	data.size[0] *= 0.9f;
	data.size[1] *= 0.9f;
	levels[game->level_count].spawn_data.add(data);

	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("C'mon...");
	levels[game->level_count].spawn_data.add(make_cross_projectile(1000));
	levels[game->level_count].spawn_data.add(make_shockwave_projectile(2000));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Driveby");
	data = make_spawner_projectile(2500, e_side_left);
	data.speed_curve = {
		.start_seconds = {0, 1},
		.end_seconds = {0.5f, 1.5f},
		.multiplier = {5, 5},
	};
	levels[game->level_count].spawn_data.add(data);

	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(2000, e_side_left));
	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(2000, e_side_right));
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(1000, e_side_left));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("4-Way");
	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(2000, e_side_left));
	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(2000, e_side_right));
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(1000, e_side_left));
	levels[game->level_count].spawn_data.add(make_basic_side_projectile(1000, e_side_right));
	levels[game->level_count].spawn_data.add(make_basic_top_projectile(1000, e_side_top));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Small Balls");

	data = make_top_diagonal_projectile(1500, e_side_left);
	data.size[0] *= 0.25f;
	data.size[1] *= 0.25f;
	levels[game->level_count].spawn_data.add(data);

	data = make_top_diagonal_projectile(1500, e_side_right);
	data.size[0] *= 0.25f;
	data.size[1] *= 0.25f;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(2000, e_side_left);
	data.size[0] *= 0.25f;
	data.size[1] *= 0.25f;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(2000, e_side_right);
	data.size[0] *= 0.25f;
	data.size[1] *= 0.25f;
	levels[game->level_count].spawn_data.add(data);

	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].infinite_jumps = true;

	set_level_name("Takeoff");

	data = make_basic_side_projectile(1000, e_side_right);
	data.y[0] = c_base_res.y;
	data.y[1] = c_base_res.y;
	levels[game->level_count].spawn_data.add(data);

	for(int i = 0; i < 5; i++)
	{
		data = make_basic_top_projectile(800, e_side_top);
		data.x[0] = c_base_res.x * (i / 4.0f);
		data.x[1] = c_base_res.x * (i / 4.0f);
		data.size[0] *= 2.6f;
		data.size[1] *= 2.6f;
		levels[game->level_count].spawn_data.add(data);
	}
	for(int i = 0; i < 5; i++)
	{
		data = make_basic_top_projectile(300, e_side_top);
		data.x[0] = c_base_res.x * (i / 4.0f) + c_base_res.x * 0.12f;
		data.x[1] = c_base_res.x * (i / 4.0f) + c_base_res.x * 0.12f;
		data.size[0] *= 2.6f;
		data.size[1] *= 2.6f;
		levels[game->level_count].spawn_data.add(data);
	}
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Volcanic Eruption");
	levels[game->level_count].infinite_jumps = true;
	levels[game->level_count].spawn_data.add(make_basic_top_projectile(20000, e_side_bottom));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Claustrophobia");

	levels[game->level_count].duration = 25;
	data = make_basic_side_projectile(3000, e_side_left);
	data.speed[0] *= 0.25f;
	data.speed[1] *= 0.25f;
	data.size[0] *= 0.25f;
	data.size[1] *= 0.25f;
	data.speed_curve = {
		.start_seconds = {0},
		.end_seconds = {0.5f},
		.multiplier = {20},
	};
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_top_projectile(3000, e_side_top);
	data.speed[0] *= 2.0f;
	data.speed[1] *= 2.0f;
	data.size[0] *= 4.0f;
	data.size[1] *= 4.0f;
	data.x[0] = c_base_res.x * 0.2f;
	data.x[1] = c_base_res.x * 0.2f;
	levels[game->level_count].spawn_data.add(data);

	data.x[0] = c_base_res.x * 0.8f;
	data.x[1] = c_base_res.x * 0.8f;
	levels[game->level_count].spawn_data.add(data);

	game->level_count++;
	// -----------------------------------------------------------------------------

	// @Note(tkap, 29/06/2023): Giant green balls
	set_level_name("Hurdling");
	data = make_basic_side_projectile(1000, e_side_left);
	data.speed[0] *= 3.0f;
	data.speed[1] *= 3.0f;
	data.size[0] *= 6.0f;
	data.size[1] *= 6.0f;
	data.y[0] = c_base_res.y * 0.9f;
	data.y[1] = c_base_res.y * 0.9f;
	levels[game->level_count].spawn_data.add(data);
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("To jump or not to jump.");
	levels[game->level_count].spawn_data.add(make_air_shot_projectile(6000));
	levels[game->level_count].spawn_data.add(make_ground_shot_projectile(4444));
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Spiral");

	levels[game->level_count].spawn_pos = v2(c_base_res.x * 0.75f, c_base_res.y);
	data = make_spiral_projectile(7777);
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(2222, e_side_left);
	data.size[0] *= 0.35f;
	data.size[1] *= 0.35f;
	levels[game->level_count].spawn_data.add(data);

	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].infinite_jumps = true;
	levels[game->level_count].spawn_pos = v2(c_base_res.x * 0.25f, c_base_res.y);
	levels[game->level_count].spawn_data.add(make_spiral_projectile(12500));

	data = make_spiral_projectile(12500);
	data.spiral_multiplier = -1;
	data.b[0] = 1;
	data.b[1] = 1;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(3111, e_side_right);
	data.y[0] = 0;
	data.size[0] *= 0.5f;
	data.size[1] *= 0.5f;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(444, e_side_right);
	data.y[0] = 0;
	data.r[0] = 1;
	data.r[1] = 1;
	data.g[0] = 0;
	data.g[1] = 0;
	data.b[0] = 0;
	data.b[1] = 0;
	levels[game->level_count].spawn_data.add(data);

	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Swirling");

	levels[game->level_count].infinite_jumps = true;
	levels[game->level_count].spawn_pos = v2(c_base_res.x * 0.25f, c_base_res.y);

	data = make_spiral_projectile(125000);
	data.set_non_random_color(1, 0, 1, 1);
	data.collide_ground_only = true;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_top_projectile(23555, e_side_top);
	data.multiply_speed(0.45f);
	data.multiply_size(0.275f);
	data.speed_curve = {
		.start_seconds = {0.1f},
		.end_seconds = {0.5f},
		.multiplier = {3},
	};
	levels[game->level_count].spawn_data.add(data);

	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Madness!");

	levels[game->level_count].reversed_controls = true;
	levels[game->level_count].background = e_background_reversed_controls;
	levels[game->level_count].spawn_data.add(make_basic_top_projectile(1234, e_side_top));

	data = make_basic_side_projectile(2850, e_side_left);
	data.multiply_speed(0.8f);
	data.multiply_size(0.5f);
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(1270, e_side_right);
	data.multiply_speed(0.5f);
	data.multiply_size(0.2f);
	levels[game->level_count].spawn_data.add(data);
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Foresight");
	data = make_basic_top_projectile(10000, e_side_top);
	data.sine_alpha = true;
	levels[game->level_count].spawn_data.add(data);
	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Rainbow");

	levels[game->level_count].background = e_background_rainbow;

	data = make_spiral_projectile(5000);
	data.spiral_multiplier = 0.35f;
	data.x[0] = data.x[1] = c_base_res.x / 4;
	data.y[0] = data.y[1] = c_base_res.y / 2;
	data.hsv_colour = true;
	data.rainbow_hue = true;
	data.r[0] = 0;
	data.r[1] = 1;
	data.g[0] = data.g[1] = 1;
	data.b[0] = data.b[1] = 1;
	data.speed_curve = {
		.start_seconds = {0.1f},
		.end_seconds = {0.25f},
		.multiplier = {2},
	};
	levels[game->level_count].spawn_data.add(data);

	data = make_spiral_projectile(4777);
	data.spiral_multiplier = 0.35f;
	data.x[0] = data.x[1] = c_base_res.x - c_base_res.x / 4;
	data.y[0] = data.y[1] = c_base_res.y / 2;
	data.hsv_colour = true;
	data.rainbow_hue = true;
	data.r[0] = 0;
	data.r[1] = 1;
	data.g[0] = data.g[1] = 1;
	data.b[0] = data.b[1] = 1;
	data.multiply_speed(1.07f);
	data.speed_curve = {
		.start_seconds = {0.1f},
		.end_seconds = {0.25f},
		.multiplier = {1.25},
	};
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(1777, e_side_left);
	data.multiply_speed(0.88f);
	data.multiply_size(0.45f);
	data.hsv_colour = true;
	data.rainbow_hue = true;
	data.r[0] = 0;
	data.r[1] = 1;
	data.g[0] = data.g[1] = 1;
	data.b[0] = data.b[1] = 1;
	levels[game->level_count].spawn_data.add(data);

	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Stray Bullet");

	data = make_basic_side_projectile(333, e_side_right);
	data.r[0] = data.r[1] = 0;
	data.g[0] = data.g[1] = 0;
	data.b[0] = data.b[1] = 0.2f;
	data.multiply_speed(3.f);
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(777, e_side_left);
	data.r[0] = data.r[1] = 0;
	data.g[0] = data.g[1] = 0.2f;
	data.b[0] = data.b[1] = 0;
	data.multiply_speed(.77f);
	levels[game->level_count].spawn_data.add(data);

	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(2800, e_side_left));

	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Choice");

	data = make_basic_top_projectile(10000, e_side_top);
	data.x[0] = data.x[1] = c_base_res.x / 2;
	data.y[0] = data.y[1] = 0;
	data.multiply_speed(3.f);
	data.multiply_size(0.5f);
	data.delay = 1.5f;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_top_projectile(10000, e_side_top);
	data.x[0] = c_base_res.x / 2;
	data.x[1] = c_base_res.x;
	data.y[0] = data.y[1] = 0;
	data.multiply_speed(0.45f);
	data.multiply_size(5.f);
	data.delay = 10.0f;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(777, e_side_left);
	data.r[0] = data.r[1] = 0;
	data.g[0] = data.g[1] = 0.2f;
	data.b[0] = data.b[1] = 0;
	data.multiply_speed(.77f);
	levels[game->level_count].spawn_data.add(data);

	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(2800, e_side_left));

	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Focus");

	levels[game->level_count].infinite_jumps = true;

	data = make_basic_side_projectile(5000, e_side_left);
	data.x[0] = data.x[1] = 0;
	data.y[0] = data.y[1] = 25;
	data.multiply_speed(3.f);
	data.multiply_size(1.25f);
	data.delay = 1.0f;
	levels[game->level_count].spawn_data.add(data);

	for(int i = 0; i < 6; i++)
	{
		data.y[0] = data.y[1] = 25.f + (35 * i);
		data.delay = 0.85f * i;
		levels[game->level_count].spawn_data.add(data);
	}

	data = make_basic_side_projectile(5000, e_side_right);
	data.x[0] = data.x[1] = c_base_res.x;
	data.y[0] = data.y[1] = c_base_res.y - 25;
	data.multiply_speed(3.f);
	data.multiply_size(1.25f);
	data.delay = 1.0f;
	levels[game->level_count].spawn_data.add(data);

	for(int i = 0; i < 6; i++)
	{
		data.y[0] = data.y[1] = c_base_res.y - (25 + (35 * i));
		data.delay = 0.85f * i;
		levels[game->level_count].spawn_data.add(data);
	}

	data = make_basic_side_projectile(555, e_side_right);
	data.x[0] = data.x[1] = c_base_res.x;
	data.y[0] = (c_base_res.y / 2) - 100;
	data.y[1] = (c_base_res.y / 2) + 100;
	data.r[0] = data.r[1] = 1;
	data.g[0] = data.g[1] = 1;
	data.b[0] = data.b[1] = 0;
	data.multiply_speed(1.25f);
	data.multiply_size(1.f);
	data.delay = 1.0f;
	levels[game->level_count].spawn_data.add(data);

	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Confusion");
	levels[game->level_count].reversed_controls = true;
	levels[game->level_count].background = e_background_reversed_controls;
	levels[game->level_count].gravity_multiplier = -1.f;

	levels[game->level_count].spawn_data.add(make_basic_top_projectile(4000, e_side_bottom));

	data = make_basic_side_projectile(1200, e_side_left);
	data.y[0] = 0;
	data.y[1] = c_base_res.y - 100;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(1200, e_side_right);
	data.y[0] = 0;
	data.y[1] = c_base_res.y - 100;
	levels[game->level_count].spawn_data.add(data);

	game->level_count++;
	// -----------------------------------------------------------------------------

	set_level_name("Moon Walk");
	levels[game->level_count].background = e_background_moon;
	levels[game->level_count].gravity_multiplier = 0.25f;

	levels[game->level_count].spawn_data.add(make_top_diagonal_projectile(3500, e_side_left));
	levels[game->level_count].spawn_data.add(make_bottom_diagonal_projectile(2500, e_side_right));

	data = make_basic_side_projectile(1200, e_side_left);
	data.y[0] = c_base_res.y * 0.35f;
	data.y[1] = c_base_res.y - 35;
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(1200, e_side_left);
	data.y[0] = data.y[1] = c_base_res.y - 35;
	data.delay = 5.0f;
	data.frequency = 4.0f;
	data.multiply_speed(1.25f);
	data.multiply_size(0.5f);
	levels[game->level_count].spawn_data.add(data);

	data = make_basic_side_projectile(1200, e_side_right);
	data.y[0] = c_base_res.y * 0.35f;
	data.y[1] = c_base_res.y - 35;
	levels[game->level_count].spawn_data.add(data);

	game->level_count++;
	// -----------------------------------------------------------------------------

	// @Note(tkap, 26/06/2023): Blank level to avoid wrapping
	game->level_count++;
}

func void reset_level(void)
{
	level_timer = 0;
	memset(spawn_timer, 0, sizeof(spawn_timer));

	// @Note(tkap, 22/06/2023): Remove projectiles
	for(int i = 0; i < c_max_entities; i++)
	{
		if(!game->e.active[i]) { continue; }
		if(game->e.type[i] == e_entity_type_projectile)
		{
			game->e.active[i] = false;
		}
	}

	s_level level = levels[game->current_level];

	// @Note(tkap, 23/06/2023): Place every player at the spawn position
	for(int i = 0; i < c_max_entities; i++)
	{
		if(!game->e.active[i]) { continue; }
		if(!game->e.player_id[i]) { continue; }

		game->e.x[i] = level.spawn_pos.x;
		game->e.y[i] = level.spawn_pos.y;
		game->e.jumping[i] = false;
		game->e.on_ground[i] = false;
		game->e.vel_y[i] = 0;
		game->e.jumps_done[i] = 1;
		handle_instant_movement(i);
	}
}

func void expire_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_expire]) { continue; }

		game->e.time_lived[ii] += delta;
		if(game->e.time_lived[ii] >= game->e.duration[ii])
		{
			game->e.active[ii] = false;
		}
	}
}

func void projectile_spawner_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_projectile_spawner]) { continue; }

		game->e.particle_spawner[ii].spawn_timer += delta;
		while(game->e.particle_spawner[ii].spawn_timer >= game->e.particle_spawner[ii].spawn_delay)
		{
			game->e.particle_spawner[ii].spawn_timer -= game->e.particle_spawner[ii].spawn_delay;

			switch(game->e.particle_spawner[ii].type)
			{
				case e_particle_spawner_default:
				{
					for(int proj_i = 0; proj_i < 2; proj_i++)
					{
						int entity = make_projectile();
						float angle = randf_range(&game->rng, pi * 0.3f, pi * 0.7f);
						game->e.x[entity] = game->e.x[ii];
						game->e.y[entity] = game->e.y[ii];
						set_size(entity, game->e.modified_sx[ii] * 0.5f, 0);
						set_speed(entity, game->e.base_speed[ii] * 0.5f);
						game->e.dir_x[entity] = cosf(angle);
						game->e.dir_y[entity] = sinf(angle);
						game->e.color[entity] = v41f(0.5f);
						handle_instant_movement(entity);
						handle_instant_resize(entity);
					}
				} break;

				case e_particle_spawner_cross:
				{
					int entity = make_projectile();
					game->e.x[entity] = game->e.x[ii];
					game->e.y[entity] = game->e.y[ii];
					set_size(entity, game->e.modified_sx[ii] * 0.5f, 0);
					set_speed(entity, game->e.base_speed[ii] * 0.5f);
					game->e.dir_x[entity] = -1.0f;
					game->e.dir_y[entity] = 0.0f;
					game->e.color[entity] = game->e.color[ii];
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					entity = make_projectile();
					game->e.x[entity] = game->e.x[ii];
					game->e.y[entity] = game->e.y[ii];
					set_size(entity, game->e.modified_sx[ii] * 0.5f, 0);
					set_speed(entity, game->e.base_speed[ii] * 0.5f);
					game->e.dir_x[entity] = 1.0f;
					game->e.dir_y[entity] = 0.0f;
					game->e.color[entity] = game->e.color[ii];
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					entity = make_projectile();
					game->e.x[entity] = game->e.x[ii];
					game->e.y[entity] = game->e.y[ii];
					set_size(entity, game->e.modified_sx[ii] * 0.5f, 0);
					set_speed(entity, game->e.base_speed[ii] * 0.5f);
					game->e.dir_x[entity] = 0.0f;
					game->e.dir_y[entity] = -1.0f;
					game->e.color[entity] = game->e.color[ii];
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					entity = make_projectile();
					game->e.x[entity] = game->e.x[ii];
					game->e.y[entity] = game->e.y[ii];
					set_size(entity, game->e.modified_sx[ii] * 0.5f, 0);
					set_speed(entity, game->e.base_speed[ii] * 0.5f);
					game->e.dir_x[entity] = 0.0f;
					game->e.dir_y[entity] = 1.0f;
					game->e.color[entity] = game->e.color[ii];
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_particle_spawner_x:
				{
					int entity = make_projectile();
					game->e.x[entity] = game->e.x[ii];
					game->e.y[entity] = game->e.y[ii];
					set_size(entity, game->e.modified_sx[ii] * 0.5f, 0);
					set_speed(entity, game->e.base_speed[ii] * 0.5f);
					game->e.dir_x[entity] = 0.5f;
					game->e.dir_y[entity] = 0.5f;
					game->e.color[entity] = game->e.color[ii];
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					entity = make_projectile();
					game->e.x[entity] = game->e.x[ii];
					game->e.y[entity] = game->e.y[ii];
					set_size(entity, game->e.modified_sx[ii] * 0.5f, 0);
					set_speed(entity, game->e.base_speed[ii] * 0.5f);
					game->e.dir_x[entity] = -0.5f;
					game->e.dir_y[entity] = -0.5f;
					game->e.color[entity] = game->e.color[ii];
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					entity = make_projectile();
					game->e.x[entity] = game->e.x[ii];
					game->e.y[entity] = game->e.y[ii];
					set_size(entity, game->e.modified_sx[ii] * 0.5f, 0);
					set_speed(entity, game->e.base_speed[ii] * 0.5f);
					game->e.dir_x[entity] = -0.5f;
					game->e.dir_y[entity] = 0.5f;
					game->e.color[entity] = game->e.color[ii];
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					entity = make_projectile();
					game->e.x[entity] = game->e.x[ii];
					game->e.y[entity] = game->e.y[ii];
					set_size(entity, game->e.modified_sx[ii] * 0.5f, 0);
					set_speed(entity, game->e.base_speed[ii] * 0.5f);
					game->e.dir_x[entity] = 0.5f;
					game->e.dir_y[entity] = -0.5f;
					game->e.color[entity] = game->e.color[ii];
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				invalid_default_case;
			}
		}
	}
}

func s_small_str str_to_name(char* str)
{
	s_small_str result = zero;
	result.len = (int)strlen(str);
	assert(result.len <= result.max_chars);
	memcpy(result.data, str, result.len + 1);
	return result;
}

func void increase_time_lived_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(game->e.dead[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_increase_time_lived]) { continue; }

		game->e.time_lived[ii] += delta;

		game->e.best_time_on_level[ii] = at_least(game->e.best_time_on_level[ii], level_timer);
	}
}

func void apply_projectile_modifiers(int entity, s_projectile_spawn_data data)
{
	game->e.speed_curve[entity] = data.speed_curve;
	game->e.size_curve[entity] = data.size_curve;

	game->e.out_of_bounds_offset[entity] = data.out_of_bounds_offset;

	if(data.sine_alpha)
	{
		game->e.flags[entity][e_entity_flag_sine_alpha] = true;
	}

	if(data.collide_ground_only)
		game->e.flags[entity][e_entity_flag_collide_ground_only] = true;

	if(data.collide_air_only)
		game->e.flags[entity][e_entity_flag_collide_air_only] = true;
}

func void set_speed(int entity, float speed)
{
	game->e.base_speed[entity] = speed;
	game->e.modified_speed[entity] = speed;
}

func void set_size(int entity, float sx, float sy)
{
	game->e.base_sx[entity] = sx;
	game->e.base_sy[entity] = sy;
	game->e.modified_sx[entity] = sx;
	game->e.modified_sy[entity] = sy;
}

func void modify_speed_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_modify_speed]) { continue; }

		assert(game->e.flags[ii][e_entity_flag_increase_time_lived]);

		int index = -1;
		for(int curve_i = 0; curve_i < 4; curve_i++)
		{
			s_float_curve curve = game->e.speed_curve[ii];
			if(floats_equal(curve.start_seconds[curve_i], 0) && floats_equal(curve.end_seconds[curve_i], 0))
			{
				break;
			}
			if(game->e.time_lived[ii] >= curve.start_seconds[curve_i] && game->e.time_lived[ii] <= curve.end_seconds[curve_i])
			{
				index = curve_i;
				break;
			}
		}
		if(index == -1)
		{
			game->e.modified_speed[ii] = game->e.base_speed[ii];
			continue;
		}

		s_float_curve sc = game->e.speed_curve[ii];
		float p = 1.0f - ilerp(sc.start_seconds[index], sc.end_seconds[index], game->e.time_lived[ii]);
		game->e.modified_speed[ii] = game->e.base_speed[ii] * (1 + sc.multiplier[index] * p);
	}
}

func void modify_size_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_modify_size]) { continue; }

		assert(game->e.flags[ii][e_entity_flag_increase_time_lived]);

		int index = -1;
		for(int curve_i = 0; curve_i < 4; curve_i++)
		{
			s_float_curve curve = game->e.size_curve[ii];
			if(floats_equal(curve.start_seconds[curve_i], 0) && floats_equal(curve.end_seconds[curve_i], 0))
			{
				break;
			}
			if(game->e.time_lived[ii] >= curve.start_seconds[curve_i] && game->e.time_lived[ii] <= curve.end_seconds[curve_i])
			{
				index = curve_i;
				break;
			}
		}
		if(index == -1)
		{
			game->e.modified_sx[ii] = game->e.base_sx[ii];
			game->e.modified_sy[ii] = game->e.base_sy[ii];
			continue;
		}

		s_float_curve sc = game->e.size_curve[ii];
		float p = 1.0f - ilerp(sc.start_seconds[index], sc.end_seconds[index], game->e.time_lived[ii]);
		game->e.modified_sx[ii] = game->e.base_sx[ii] * (1 + sc.multiplier[index] * p);
		game->e.modified_sy[ii] = game->e.base_sy[ii] * (1 + sc.multiplier[index] * p);
	}
}

func s_projectile_spawn_data make_basic_top_projectile(float speed, e_side side)
{
	float y = side == e_side_top ? -c_projectile_spawn_offset : c_base_res.y + c_projectile_spawn_offset;
	float angle = side == e_side_top ? 0.25f : -0.25f;
	return {
		.frequency = m_speed(speed),
		.x = {0, c_base_res.x},
		.y = {y, y},
		.speed = {400, 500},
		.size = {48, 56},
		.angle = {angle, angle},
		.r = {0.9f, 0.9f},
		.g = {0.1f, 0.1f},
		.b = {0.1f, 0.1f},
		.a = {1, 1},
	};
}

func s_projectile_spawn_data make_spiral_projectile(float speed)
{
	return {
		.frequency = m_speed(speed),
		.x = m_twice(c_base_res.x / 2),
		.y = m_twice(c_base_res.y / 2),
		.speed = m_twice(300),
		.size = m_twice(22),
		.r = m_twice(1),
		.g = m_twice(1),
		.b = m_twice(0),
		.a = m_twice(1),
		.spiral_multiplier = 1,
		.on_spawn = e_on_spawn_spiral,
	};
}

func s_projectile_spawn_data make_shockwave_projectile(float speed)
{
	return {
		.frequency = m_speed(speed),
		.x = {0, c_base_res.x},
		.y = {0, c_base_res.y / 3},
		.speed = {125, 455},
		.size = m_twice(66),
		.angle = m_twice(0.25f),
		.r = {0.05f, 1.0f},
		.g = {0.85f, 1.0f},
		.b = {0.05f, 1.0f},
		.a = m_twice(1),
		.on_spawn = e_on_spawn_shockwave,
	};
}

func s_projectile_spawn_data make_corner_shot_projectile(float speed)
{
	return {
		.frequency = m_speed(speed),
		.y = m_twice(0),
		.size = m_twice(300),
		.r = {0.775f, 1.0f},
		.g = m_twice(0),
		.b = m_twice(0),
		.a = m_twice(1),
		.on_spawn = e_on_spawn_corner_shot,
	};
}

func s_projectile_spawn_data make_basic_side_projectile(float speed, e_side side)
{
	float x = side == e_side_left ? -c_projectile_spawn_offset : c_base_res.x + c_projectile_spawn_offset;
	float angle = side == e_side_left ? 0 : 0.5f;
	return {
		.frequency = m_speed(speed),
		.x = {x, x},
		.y = {c_base_res.y * 0.6f, c_base_res.y},
		.speed = {400, 500},
		.size = {38, 46},
		.angle = {angle, angle},
		.r = {0.1f, 0.1f},
		.g = {0.9f, 0.9f},
		.b = {0.1f, 0.1f},
		.a = {1, 1},
	};
}

func s_projectile_spawn_data make_spawner_projectile(float speed, e_side side)
{
	float x = side == e_side_left ? -c_projectile_spawn_offset : c_base_res.x + c_projectile_spawn_offset;
	float angle = side == e_side_left ? 0 : 0.5f;
	return {
		.frequency = m_speed(speed),
		.x = m_twice(x),
		.y = m_twice(c_base_res.y * 0.5f),
		.speed = m_twice(300),
		.size = m_twice(50),
		.angle = m_twice(angle),
		.r = m_twice(0.1f),
		.g = m_twice(0.1f),
		.b = m_twice(0.9f),
		.a = m_twice(1),
		.on_spawn = e_on_spawn_spawner,
	};
}

func s_projectile_spawn_data make_top_diagonal_projectile(float speed, e_side side)
{
	s_v2 pos = v2(
		side == e_side_left ? 0 : c_base_res.x,
		0
	);
	float opposite_x = side == e_side_left ? c_base_res.x : 0;
	s_v2 a = v2(opposite_x, c_base_res.y * 0.7f) - pos;
	s_v2 b = v2(pos.x, c_base_res.y) - pos;

	return {
		.frequency = m_speed(speed),
		.x = {pos.x, pos.x},
		.y = {pos.y, pos.y},
		.speed = {400, 500},
		.size = {38, 46},
		.angle = {v2_angle(a) / tau, v2_angle(b) / tau},
		.r = {0.9f, 0.9f},
		.g = {0.9f, 0.9f},
		.b = {0.1f, 0.1f},
		.a = {1, 1},
	};
}

func s_projectile_spawn_data make_bottom_diagonal_projectile(float speed, e_side side)
{
	float angle = side == e_side_left ? -0.125f : 0.625f;
	float x = side == e_side_left ? 0 : c_base_res.x;

	return {
		.frequency = m_speed(speed),
		.x = {x, x},
		.y = {c_base_res.y, c_base_res.y},
		.speed = {400, 500},
		.size = {38, 46},
		.angle = {angle, angle},
		.r = {0.1f, 0.1f},
		.g = {0.4f, 0.4f},
		.b = {0.4f, 0.4f},
		.a = {1, 1},
		.on_spawn = e_on_spawn_bottom_diagonal,
	};
}

func s_projectile_spawn_data make_ground_shot_projectile(float speed)
{
	return {
		.frequency = m_speed(speed),
		.x = {c_base_res.x + c_projectile_spawn_offset, c_base_res.x + c_projectile_spawn_offset},
		.y = {c_base_res.y - 250, c_base_res.y},
		.speed = {444, 444},
		.size = {50, 50},
		.angle = {0.5f, 0.5f},
		.r = {0, 0},
		.g = {1, 1},
		.b = {1, 1},
		.a = {1, 1},
		.collide_ground_only = true,
	};
}

func s_projectile_spawn_data make_air_shot_projectile(float speed)
{
	return {
		.frequency = m_speed(speed),
		.x = {0, c_base_res.x * 2},
		.y = {-c_projectile_spawn_offset, -c_projectile_spawn_offset},
		.speed = {444, 444},
		.size = {50, 50},
		.angle = {0.375f, 0.375f},
		.r = {1, 1},
		.g = {0, 0},
		.b = {1, 1},
		.a = {1, 1},
		.collide_air_only = true,
		.out_of_bounds_offset = 2000,
	};
}

func s_projectile_spawn_data make_cross_projectile(float speed)
{
	return {
		.frequency = m_speed(speed),
		.speed = {125, 255},
		.size = {15, 44},
		.r = {0, 1},
		.g = {0, 1},
		.b = {0, 1},
		.a = {1, 1},
		.on_spawn = e_on_spawn_cross,
	};
}

func void on_spawn(int entity, s_projectile_spawn_data data)
{
	switch(data.on_spawn)
	{
		case e_on_spawn_spiral:
		{
			float angle = data.spiral_offset + level_timer * data.spiral_multiplier * tau;
			s_v2 dir = v2_from_angle(angle);
			game->e.dir_x[entity] = dir.x;
			game->e.dir_y[entity] = dir.y;
		} break;

		case e_on_spawn_cross:
		{
			float x;
			float y;
			float speed = game->e.modified_speed[entity];
			if(rand_bool(&game->rng))
			{
				x = randf_range(&game->rng, 0, c_base_res.x / 6);
			}
			else
			{
				x = randf_range(&game->rng, c_base_res.x - c_base_res.x / 6, c_base_res.x);
				speed *= randf_range(&game->rng, 1.5f, 2.5f);
			}
			if(rand_bool(&game->rng))
			{
				y = randf_range(&game->rng, c_base_res.y - c_base_res.y / 8, c_base_res.y);
			}
			else
			{
				y = randf_range(&game->rng, 0, c_base_res.y);
			}

			int entities[4];
			entities[0] = entity;
			for(int i = 0; i < 3; i++)
			{
				entities[i + 1] = make_projectile();
				set_size(entities[i + 1], game->e.modified_sx[entity], 0);
				game->e.color[entities[i + 1]] = game->e.color[entity];
			}

			float speed_arr[] = {speed, speed, 133, 133};
			s_v2 dir_arr[] = {v2(-1, 0), v2(1, 0), v2(0, -1), v2(0, 1)};
			for(int i = 0; i < 4; i++)
			{
				int e = entities[i];
				game->e.x[e] = x;
				game->e.y[e] = y;
				game->e.dir_x[e] = dir_arr[i].x;
				game->e.dir_y[e] = dir_arr[i].y;
				set_speed(e, speed_arr[i]);
				apply_projectile_modifiers(e, data);
				handle_instant_movement(e);
				handle_instant_resize(e);
			}
		} break;

		case e_on_spawn_bottom_diagonal:
		{
			game->e.flags[entity][e_entity_flag_move] = false;
			game->e.flags[entity][e_entity_flag_physics_movement] = true;
			game->e.flags[entity][e_entity_flag_gravity] = true;

			s_v2 vel = v2_mul(v2_from_angle(data.angle[0] * tau), randf_range(&game->rng, 200, 2000));
			game->e.vel_x[entity] = vel.x;
			game->e.vel_y[entity] = vel.y;

		} break;

		case e_on_spawn_spawner:
		{
			game->e.flags[entity][e_entity_flag_projectile_spawner] = true;
			game->e.particle_spawner[entity].type = e_particle_spawner_default;
			game->e.particle_spawner[entity].spawn_delay = 1.0f;
		} break;

		case e_on_spawn_corner_shot:
		{
			float size = chance100(&game->rng, 6) ? 200.f : randf_range(&game->rng, 45, 65);
			float speed = randf_range(&game->rng, 166, 311);

			int shock_proj_num = rand_range_ii(&game->rng, 7, 14);
			float inc = tau / shock_proj_num;

			float x = rand_bool(&game->rng) ? 0 : c_base_res.x;
			game->e.x[entity] = x;
			handle_instant_movement(entity);

			float y = game->e.y[entity];
			s_v4 color = game->e.color[entity];

			for(int shock_proj_i = 0; shock_proj_i < shock_proj_num; shock_proj_i++)
			{
				int new_entity = make_projectile();
				game->e.x[new_entity] = x;
				game->e.y[new_entity] = y;
				set_size(new_entity, size, 0);
				set_speed(new_entity, speed);
				game->e.dir_x[new_entity] = sinf(shock_proj_i * inc * tau);
				game->e.dir_y[new_entity] = cosf(shock_proj_i * inc * tau);
				game->e.color[new_entity] = color;
				apply_projectile_modifiers(new_entity, data);
				handle_instant_movement(new_entity);
				handle_instant_resize(new_entity);
			}
		} break;

		case e_on_spawn_shockwave:
		{
			int shock_proj_num = rand_range_ii(&game->rng, 7, 14);
			float inc = tau / shock_proj_num;
			float size = randf_range(&game->rng, 15, 55);
			float speed = randf_range(&game->rng, 166, 311);

			float x = game->e.x[entity];
			float y = game->e.y[entity];
			s_v4 color = game->e.color[entity];

			for(int shock_proj_i = 0; shock_proj_i < shock_proj_num; shock_proj_i++)
			{
				int new_entity = make_projectile();
				game->e.x[new_entity] = x;
				game->e.y[new_entity] = y;
				set_size(new_entity, size, 0);
				set_speed(new_entity, speed);
				game->e.dir_x[new_entity] = sinf(shock_proj_i * inc * 2 * pi);
				game->e.dir_y[new_entity] = cosf(shock_proj_i * inc * 2 * pi);
				game->e.color[new_entity] = color;
				apply_projectile_modifiers(new_entity, data);
				handle_instant_movement(new_entity);
				handle_instant_resize(new_entity);
			}
		} break;

		invalid_default_case;
	}
}



func void start_dash(int entity)
{
	game->e.can_dash[entity] = false;
	game->e.dashing[entity] = true;
	game->e.dash_timer[entity] = 0;
	game->e.trail_timer[entity] = c_dash_duration * 4.0f;
}

func void cancel_dash(int entity)
{
	game->e.dashing[entity] = false;
	game->e.dash_timer[entity] = 0;
}
