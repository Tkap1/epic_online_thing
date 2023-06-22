


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
	e.dir_y[index] = 0;
	e.player_id[index] = 0;
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