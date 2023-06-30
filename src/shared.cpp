
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

		game->e.x[ii] += game->e.dir_x[ii] * game->e.modified_speed[ii] * delta;
		game->e.y[ii] += game->e.vel_y[ii] * delta;

		if(game->e.vel_y[ii] >= 0) { game->e.jumping[ii] = false; }
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
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_gravity]) { continue; }

		game->e.vel_y[ii] += c_gravity * delta;
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
			game->e.x[ii] + radius < -c_projectile_out_of_bounds_offset ||
			game->e.y[ii] + radius < -c_projectile_out_of_bounds_offset ||
			game->e.x[ii] - radius > c_base_res.x + c_projectile_out_of_bounds_offset ||
			game->e.y[ii] - radius > c_base_res.y + c_projectile_out_of_bounds_offset
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
		while(spawn_timer[spawn_i] >= data.delay)
		{
			spawn_timer[spawn_i] -= data.delay;

			int entity = make_projectile();
			switch(data.type)
			{
				case e_projectile_type_top_basic:
				{
					game->e.x[entity] = randf32(&game->rng) * c_base_res.x;
					game->e.y[entity] = -c_projectile_spawn_offset;
					game->e.dir_y[entity] = 1;
					set_speed(entity, randf_range(&game->rng, 400, 500));
					set_size(entity, randf_range(&game->rng, 48, 56), 0);
					game->e.color[entity] = v4(0.9f, 0.1f, 0.1f, 1.0f);

					if(data.x_override != c_max_f32)
					{
						game->e.x[entity] = data.x_override;
					}

					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_bottom_basic:
				{
					game->e.x[entity] = randf32(&game->rng) * c_base_res.x;
					game->e.y[entity] = c_base_res.y + c_projectile_spawn_offset * 2;
					game->e.dir_y[entity] = -1;
					set_speed(entity, randf_range(&game->rng, 400, 500));
					set_size(entity, randf_range(&game->rng, 48, 56), 0);
					game->e.color[entity] = v4(0.9f, 0.1f, 0.1f, 1.0f);

					if(data.x_override != c_max_f32)
					{
						game->e.x[entity] = data.x_override;
					}

					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_left_basic:
				{
					make_side_projectile(entity, -c_projectile_spawn_offset, 1);
					if(data.y_override != c_max_f32)
					{
						game->e.y[entity] = data.y_override;
					}
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_right_basic:
				{
					make_side_projectile(entity, c_base_res.x + c_projectile_spawn_offset, -1);
					if(data.y_override != c_max_f32)
					{
						game->e.y[entity] = data.y_override;
					}
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_diagonal_left:
				{
					make_diagonal_top_projectile(entity, 0, c_base_res.x);
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_diagonal_right:
				{
					make_diagonal_top_projectile(entity, c_base_res.x, 0);
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_diagonal_bottom_left:
				{
					float angle = pi * -0.25f;
					make_diagonal_bottom_projectile(entity, 0.0f, angle);
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_diagonal_bottom_right:
				{
					float angle = pi * -0.75f;
					make_diagonal_bottom_projectile(entity, c_base_res.x, angle);
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_right_full_height:
				{
					game->e.x[entity] = c_base_res.x + c_projectile_spawn_offset;
					game->e.y[entity] = randf_range(&game->rng, 0, c_base_res.y);
					game->e.dir_x[entity] = -1;
					set_speed(entity, randf_range(&game->rng, 400, 500));
					set_size(entity, randf_range(&game->rng, 38, 46), 0);
					game->e.color[entity] = v4(0.1f, 0.9f, 0.1f, 1.0f);
					if(data.y_override != c_max_f32)
					{
						game->e.y[entity] = data.y_override;
					}
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_corner_shot:
				{
					float x = (randu(&game->rng) & 1) ? 0 : c_base_res.x;
					float y = 0;
					float size = (rand_range_ii(&game->rng, 0, 99) < 6) ? 200.f : randf_range(&game->rng, 45, 65);
					float speed = randf_range(&game->rng, 166, 311);
					s_v4 col = v4(randf_range(&game->rng, 0.775f, 1.0f), 0.0f, 0.0f, 1.0f);

					game->e.x[entity] = x;
					game->e.y[entity] = y;
					set_size(entity, 300.0f, 0);
					set_speed(entity, randf_range(&game->rng, 125, 455));
					game->e.dir_x[entity] = 0.0f;
					game->e.dir_y[entity] = 0.0f;
					game->e.color[entity] = col;
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					int shock_proj_num = rand_range_ii(&game->rng, 7, 14);
					float inc = deg_to_rad(360.f / shock_proj_num);

					for(int shock_proj_i = 0; shock_proj_i < shock_proj_num; shock_proj_i++)
					{
						entity = make_projectile();
						game->e.x[entity] = x;
						game->e.y[entity] = y;
						set_size(entity, size, 0);
						set_speed(entity, speed);
						game->e.dir_x[entity] = sinf(shock_proj_i * inc * 2 * pi);
						game->e.dir_y[entity] = cosf(shock_proj_i * inc * 2 * pi);
						game->e.color[entity] = col;
						apply_projectile_modifiers(entity, data);
						handle_instant_movement(entity);
						handle_instant_resize(entity);
					}
				} break;

				case e_projectile_type_shockwave:
				{
					float x = randf_range(&game->rng, 0, c_base_res.x);
					float y = randf_range(&game->rng, 0, c_base_res.y / 3);
					s_v4 col = v4(randf_range(&game->rng, 0.05f, 1.0f), randf_range(&game->rng, 0.85f, 1.0f), randf_range(&game->rng, .05f, 1.0f), 1.0f);

					game->e.x[entity] = x;
					game->e.y[entity] = y;
					set_size(entity, 66.f, 0);
					set_speed(entity, randf_range(&game->rng, 125, 455));
					game->e.dir_x[entity] = 0.0f;
					game->e.dir_y[entity] = 1.0f;
					game->e.color[entity] = col;
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					int shock_proj_num = rand_range_ii(&game->rng, 7, 14);
					float inc = deg_to_rad(360.f / shock_proj_num);
					float size = randf_range(&game->rng, 15, 55);
					float speed = randf_range(&game->rng, 166, 311);

					for(int shock_proj_i = 0; shock_proj_i < shock_proj_num; shock_proj_i++)
					{
						entity = make_projectile();
						game->e.x[entity] = x;
						game->e.y[entity] = y;
						set_size(entity, size, 0);
						set_speed(entity, speed);
						game->e.dir_x[entity] = sinf(shock_proj_i * inc * 2 * pi);
						game->e.dir_y[entity] = cosf(shock_proj_i * inc * 2 * pi);
						game->e.color[entity] = col;
						apply_projectile_modifiers(entity, data);
						handle_instant_movement(entity);
						handle_instant_resize(entity);
					}
				} break;

				case e_projectile_type_cross:
				{
					float x;
					float y;
					float size = randf_range(&game->rng, 15, 44);
					float speed = randf_range(&game->rng, 125, 255);

					if((randu(&game->rng) & 1) == 0)
					{
						x = randf_range(&game->rng, 0, c_base_res.x / 6);
					}
					else
					{
						x = randf_range(&game->rng, c_base_res.x - c_base_res.x / 6, c_base_res.x);
						speed *= randf_range(&game->rng, 1.5f, 2.5f);
					}

					if((randu(&game->rng) & 1) == 0)
					{
						y = randf_range(&game->rng, c_base_res.y - c_base_res.y / 8, c_base_res.y);
					}
					else
					{
						y = randf_range(&game->rng, 0, c_base_res.y);
					}

					s_v4 col = v4(randf_range(&game->rng, 0, 1.0f), randf_range(&game->rng, 0, 1.0f), randf_range(&game->rng, 0, 1.0f), 1.0f);

					game->e.x[entity] = x;
					game->e.y[entity] = y;
					set_size(entity, size, 0);
					set_speed(entity, speed);
					game->e.dir_x[entity] = -1.0f;
					game->e.dir_y[entity] = 0.0f;
					game->e.color[entity] = col;
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					entity = make_projectile();
					game->e.x[entity] = x;
					game->e.y[entity] = y;
					set_size(entity, size, 0);
					set_speed(entity, speed);
					game->e.dir_x[entity] = 1.0f;
					game->e.dir_y[entity] = 0.0f;
					game->e.color[entity] = col;
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					entity = make_projectile();
					game->e.x[entity] = x;
					game->e.y[entity] = y;
					set_size(entity, size, 0);
					set_speed(entity, 133);
					game->e.dir_x[entity] = 0.0f;
					game->e.dir_y[entity] = -1.0f;
					game->e.color[entity] = col;
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);

					entity = make_projectile();
					game->e.x[entity] = x;
					game->e.y[entity] = y;
					set_size(entity, size, 0);
					set_speed(entity, 133);
					game->e.dir_x[entity] = 0.0f;
					game->e.dir_y[entity] = 1.0f;
					game->e.color[entity] = col;
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_ground_shot:
				{
					float x = c_base_res.x + c_projectile_spawn_offset;
					float y = randf_range(&game->rng, c_base_res.y - 250, c_base_res.y);

					game->e.x[entity] = x;
					game->e.y[entity] = y;
					set_size(entity, 50, 0);
					set_speed(entity, 444);
					game->e.dir_x[entity] = -1.0f;
					game->e.dir_y[entity] = 0.0f;
					game->e.color[entity] = v4(0.0f, 1.0f, 1.0f, 1.0f);
					game->e.flags[entity][e_entity_flag_collide_ground_only] = true;
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_air_shot:
				{
					float x = randf_range(&game->rng, 0, c_base_res.x * 2);
					float y = -c_projectile_spawn_offset;

					game->e.x[entity] = x;
					game->e.y[entity] = y;
					set_size(entity, 50, 0);
					set_speed(entity, 444);
					game->e.dir_x[entity] = -0.5f;
					game->e.dir_y[entity] = 0.5f;
					game->e.color[entity] = v4(1.0f, 0.0f, 1.0f, 1.0f);
					game->e.flags[entity][e_entity_flag_collide_air_only] = true;
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_spiral:
				{
					float x = c_base_res.x / 2;
					float y = c_base_res.y / 2;
					float t = fract(level_timer) * data.spiral_multiplier * 2 * pi;

					game->e.x[entity] = x;
					game->e.y[entity] = y;
					set_size(entity, 22, 0);
					set_speed(entity, 300);
					game->e.dir_x[entity] = sinf(t);
					game->e.dir_y[entity] = cosf(t);
					game->e.color[entity] = v4(1.0f, 1.0f, 0.0f, 1.0f);
					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				case e_projectile_type_spawner:
				{
					game->e.x[entity] = -c_projectile_spawn_offset;
					game->e.y[entity] = c_base_res.y * 0.5f;
					game->e.dir_x[entity] = 1;
					set_speed(entity, 300);
					set_size(entity, 50, 0);
					game->e.color[entity] = v4(0.1f, 0.1f, 0.9f, 1.0f);

					game->e.flags[entity][e_entity_flag_projectile_spawner] = true;
					game->e.particle_spawner[entity].type = e_particle_spawner_default;
					game->e.particle_spawner[entity].spawn_delay = 1.0f;

					apply_projectile_modifiers(entity, data);
					handle_instant_movement(entity);
					handle_instant_resize(entity);
				} break;

				invalid_default_case;
			}
		}
	}
}

func void init_levels(void)
{
	game->level_count = 0;
	memset(levels, 0, sizeof(levels));

	#define speed(val) (1000.0f / val)

	s_v2 default_spawn_position = v2(c_base_res.x * 0.5f, c_base_res.y * 0.5f);

	for(int level_i = 0; level_i < c_max_levels; level_i++)
	{
		levels[level_i].spawn_pos = default_spawn_position;
		levels[level_i].duration = c_level_duration;
		levels[level_i].infinite_jumps = false;
		levels[level_i].background = e_background_default;
	}

	// -----------------------------------------------------------------------------
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_top_basic,
		.delay = speed(4000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(2000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_right,
		.delay = speed(3000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_basic,
		.delay = speed(1000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_ground_shot,
		.delay = speed(3500),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_left,
		.delay = speed(2800),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_air_shot,
		.delay = speed(10000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_bottom_right,
		.delay = speed(2800),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_spawner,
		.delay = speed(1000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_bottom_left,
		.delay = speed(3300),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(1000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_basic,
		.delay = speed(1000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_cross,
		.delay = speed(2777),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_top_basic,
		.delay = speed(3000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(2500),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_left,
		.delay = speed(3500),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_bottom_right,
		.delay = speed(2500),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_top_basic,
		.delay = speed(2000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_spawner,
		.delay = speed(1500),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_basic,
		.delay = speed(2200),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_bottom_left,
		.delay = speed(1000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_left,
		.delay = speed(4000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_right,
		.delay = speed(4000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_top_basic,
		.delay = speed(4000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(1200),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_basic,
		.delay = speed(1200),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_bottom_left,
		.delay = speed(2200),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_bottom_right,
		.delay = speed(2200),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_corner_shot,
		.delay = speed(3333),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_shockwave,
		.delay = speed(2777),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_cross,
		.delay = speed(1000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_shockwave,
		.delay = speed(2000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	// @Note(tkap, 25/06/2023): Maze
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_top_basic,
		.delay = speed(25000),
		.speed_multiplier = 0.33f,
		.size_multiplier = 0.25f,
		.speed_curve = {
			.start_seconds = {0},
			.end_seconds = {1},
			.multiplier = {5},
		},
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(2000),
		.speed_multiplier = 1.5f,
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_basic,
		.delay = speed(1400),
		.speed_multiplier = 0.25f,
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	// @Note(tkap, 26/06/2023): Can't be on bottom, infinite jump
	levels[game->level_count].infinite_jumps = true;
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(3000),
		.speed_multiplier = 2.0f,
		.y_override = c_base_res.y * 0.96f,
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_basic,
		.delay = speed(3000),
		.speed_multiplier = 2.0f,
		.y_override = c_base_res.y * 0.96f,
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(1000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_basic,
		.delay = speed(1000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_top_basic,
		.delay = speed(8000),
		.size_multiplier = 0.9f,
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_spawner,
		.delay = speed(2500),
		.speed_curve = {
			.start_seconds = {0, 1},
			.end_seconds = {0.5f, 1.5f},
			.multiplier = {5, 5},
		},
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_left,
		.delay = speed(2000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_right,
		.delay = speed(2000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(1000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_left,
		.delay = speed(2000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_right,
		.delay = speed(2000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(1000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_basic,
		.delay = speed(1000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_top_basic,
		.delay = speed(1000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_left,
		.delay = speed(1500),
		.size_multiplier = 0.25f,
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_diagonal_right,
		.delay = speed(1500),
		.size_multiplier = 0.25f,
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(2000),
		.size_multiplier = 0.25f,
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_basic,
		.delay = speed(2000),
		.size_multiplier = 0.25f,
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].infinite_jumps = true;
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(1000),
		.y_override = c_base_res.y,
	});
	for(int i = 0; i < 5; i++)
	{
		levels[game->level_count].spawn_data.add({
			.type = e_projectile_type_top_basic,
			.delay = speed(800),
			.size_multiplier = 2.6f,
			.x_override = c_base_res.x * (i / 4.0f),
		});
	}
	for(int i = 0; i < 5; i++)
	{
		levels[game->level_count].spawn_data.add({
			.type = e_projectile_type_top_basic,
			.delay = speed(300),
			.size_multiplier = 2.6f,
			.x_override = c_base_res.x * (i / 4.0f) + c_base_res.x * 0.12f,
		});
	}
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].infinite_jumps = true;
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_bottom_basic,
		.delay = speed(20000),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].duration = 25;
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(3000),
		.speed_multiplier = 0.25f,
		.size_multiplier = 0.25f,
		.speed_curve = {
			.start_seconds = {0},
			.end_seconds = {0.5f},
			.multiplier = {20},
		},
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_top_basic,
		.delay = speed(3000),
		.speed_multiplier = 2.0f,
		.size_multiplier = 4.0f,
		.x_override = 0.2f * c_base_res.x,
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_top_basic,
		.delay = speed(3000),
		.speed_multiplier = 2.0f,
		.size_multiplier = 4.0f,
		.x_override = 0.8f * c_base_res.x,
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	// @Note(tkap, 29/06/2023): Giant green balls
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(1000),
		.speed_multiplier = 3.0f,
		.size_multiplier = 6.0f,
		.y_override = c_base_res.y * 0.9f,
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_air_shot,
		.delay = speed(6000),
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_ground_shot,
		.delay = speed(4444),
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].spawn_pos = v2(c_base_res.x * 0.75f, c_base_res.y);
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_spiral,
		.delay = speed(7777),
		.spiral_multiplier = 1,
		.color_overide = v4(1.0f, 1.0f, 0.0f, 1.0f)
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_left_basic,
		.delay = speed(2222),
		.size_multiplier = 0.35f,
		.color_overide = v4(1.0f, 0.1f, 0.1f, 1.0f)
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	levels[game->level_count].infinite_jumps = true;
	levels[game->level_count].spawn_pos = v2(c_base_res.x * 0.25f, c_base_res.y);
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_spiral,
		.delay = speed(12500),
		.spiral_multiplier = 1,
		.color_overide = v4(1.0f, 1.0f, 0.0f, 1.0f)
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_spiral,
		.delay = speed(12500),
		.spiral_multiplier = -1,
		.color_overide = v4(1.0f, 1.0f, 1.0f, 1.0f)
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_full_height,
		.delay = speed(3111),
		.size_multiplier = 0.5f,
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_right_full_height,
		.delay = speed(444),
		.size_multiplier = 1.0f,
		.color_overide = v4(1.0f, 0.0f, 0.0f, 1.0f)
	});
	game->level_count++;
	// -----------------------------------------------------------------------------
	levels[game->level_count].infinite_jumps = true;
	levels[game->level_count].spawn_pos = v2(c_base_res.x * 0.25f, c_base_res.y);
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_spiral,
		.delay = speed(125000),
		.spiral_multiplier = 1,
		.color_overide = v4(1.0f, 0.0f, 1.0f, 1.0f),
		.collide_ground_only = true
	});
	levels[game->level_count].spawn_data.add({
		.type = e_projectile_type_top_basic,
		.delay = speed(23555),
		.speed_multiplier = 0.45f,
		.size_multiplier = 0.275f,
		.speed_curve = {
			.start_seconds = {0.1f},
			.end_seconds = {0.5f},
			.multiplier = {3},
		},
	});
	game->level_count++;
	// -----------------------------------------------------------------------------

	// @Note(tkap, 26/06/2023): Blank level to avoid wrapping
	game->level_count++;

	#undef speed
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

func void make_diagonal_bottom_projectile(int entity, float x, float angle)
{
	game->e.x[entity] = x;
	game->e.y[entity] = c_base_res.y;
	s_v2 vel = v2_mul(v2_from_angle(angle), randf_range(&game->rng, 200, 2000));
	game->e.vel_x[entity] = vel.x;
	game->e.vel_y[entity] = vel.y;
	set_size(entity, randf_range(&game->rng, 38, 46), 0);
	game->e.color[entity] = v4(0.1f, 0.4f, 0.4f, 1.0f);
	game->e.flags[entity][e_entity_flag_physics_movement] = true;
	game->e.flags[entity][e_entity_flag_move] = false;
	game->e.flags[entity][e_entity_flag_gravity] = true;
}

func void make_diagonal_top_projectile(int entity, float x, float opposite_x)
{
	s_v2 pos = v2(x, 0.0f);
	game->e.x[entity] = pos.x;
	game->e.y[entity] = pos.y;
	s_v2 a = v2_sub(v2(opposite_x, c_base_res.y * 0.7f), pos);
	s_v2 b = v2_sub(v2(x, c_base_res.y), pos);
	float angle = randf_range(&game->rng, v2_angle(a), v2_angle(b));
	s_v2 dir = v2_from_angle(angle);
	game->e.dir_x[entity] = dir.x;
	game->e.dir_y[entity] = dir.y;
	set_speed(entity, randf_range(&game->rng, 400, 500));
	set_size(entity, randf_range(&game->rng, 38, 46), 0);
	game->e.color[entity] = v4(0.9f, 0.9f, 0.1f, 1.0f);
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

func void make_side_projectile(int entity, float x, float x_dir)
{
	game->e.x[entity] = x;
	game->e.y[entity] = randf_range(&game->rng, c_base_res.y * 0.6f, c_base_res.y);
	game->e.dir_x[entity] = x_dir;
	set_speed(entity, randf_range(&game->rng, 400, 500));
	set_size(entity, randf_range(&game->rng, 38, 46), 0);
	game->e.color[entity] = v4(0.1f, 0.9f, 0.1f, 1.0f);
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
	game->e.base_sx[entity] *= data.size_multiplier;
	game->e.base_sy[entity] *= data.size_multiplier;
	game->e.modified_sx[entity] *= data.size_multiplier;
	game->e.modified_sy[entity] *= data.size_multiplier;

	game->e.base_speed[entity] *= data.speed_multiplier;
	game->e.modified_speed[entity] *= data.speed_multiplier;

	game->e.speed_curve[entity] = data.speed_curve;
	game->e.size_curve[entity] = data.size_curve;

	if (data.color_overide.x != c_max_f32)
		game->e.color[entity].x = data.color_overide.x;

	if (data.color_overide.y != c_max_f32)
		game->e.color[entity].y = data.color_overide.y;

	if (data.color_overide.z != c_max_f32)
		game->e.color[entity].z = data.color_overide.z;

	if (data.color_overide.w != c_max_f32)
		game->e.color[entity].w = data.color_overide.w;

	if (data.collide_ground_only)
		game->e.flags[entity][e_entity_flag_collide_ground_only] = true;

	if (data.collide_air_only)
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
