#define m_client 1


#ifdef _WIN32
// @Note(tkap, 24/06/2023): We don't want this Madeg
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // _WIN32

#include <GL/gl.h>
#include "external/glcorearb.h"
#include "external/wglext.h"

#include <winsock2.h>
#include <stdio.h>
#include <math.h>
#include "external/enet/enet.h"
#include "types.h"
#include "utils.h"
#include "math.h"
#include "config.h"
#include "shared.h"
#include "memory.h"
#include "file.h"
#include "rng.h"
#include "platform_shared.h"
#include "client.h"
#include "shader_shared.h"
#include "str_builder.h"
#include "audio.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert assert
#include "external/stb_truetype.h"

global s_sarray<s_transform, c_max_entities> transforms;
global s_sarray<s_transform, c_max_entities> text_arr[e_font_count];

global s_entities e;
global u32 my_id = 0;
global ENetPeer* server;
global s_lin_arena frame_arena;
global s_rng rng;
global s_font g_font_arr[e_font_count];
global e_state state;
global b8 g_connected;
global ENetHost* g_client;
global s_main_menu main_menu;
global u32 g_program;
global float total_time;

global s_sound big_dog_sound = zero;
global s_sound jump_sound = zero;
global s_sound jump2_sound = zero;
global s_sound win_sound = zero;

global int level_count = 0;

global s_game_window g_window;
global s_input* g_input;
global s_sarray<s_char_event, 1024>* char_event_arr;

global b8 game_initialized;
global s_platform_data g_platform_data;
global s_platform_funcs g_platform_funcs;

global s_game game;


#ifdef _WIN32
#define X(type, name) global type name = null;
m_gl_funcs
#undef X
#endif // _WIN32

#include "draw.cpp"
#include "memory.cpp"
#include "file.cpp"
#include "shared.cpp"
#include "str_builder.cpp"
#include "audio.cpp"


void update_game(s_platform_data platform_data, s_platform_funcs platform_funcs)
{
	assert((c_max_entities % c_num_threads) == 0);

	g_platform_funcs = platform_funcs;
	g_platform_data = platform_data;
	char_event_arr = platform_data.char_event_arr;
	g_input = platform_data.input;
	if(!game_initialized)
	{

		#define X(type, name) name = (type)platform_funcs.load_gl_func(#name);
		m_gl_funcs
		#undef X

		glDebugMessageCallback(gl_debug_callback, null);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		rng.seed = (u32)__rdtsc();
		frame_arena = make_lin_arena(10 * c_mb);

		game.config = read_config_or_make_default(&frame_arena, &rng);

		if(wglSwapIntervalEXT)
		{
			wglSwapIntervalEXT(1);
		}

		game_initialized = true;
		init_levels();

		jump_sound = load_wav("assets/jump.wav", &frame_arena);
		jump2_sound = load_wav("assets/jump2.wav", &frame_arena);
		big_dog_sound = load_wav("assets/big_dog.wav", &frame_arena);
		win_sound = load_wav("assets/win.wav", &frame_arena);

		g_font_arr[e_font_small] = load_font("assets/consola.ttf", 24, &frame_arena);
		g_font_arr[e_font_medium] = load_font("assets/consola.ttf", 36, &frame_arena);
		g_font_arr[e_font_big] = load_font("assets/consola.ttf", 72, &frame_arena);

		u32 vao;
		u32 ssbo;
		g_program = load_shader("shaders/vertex.vertex", "shaders/fragment.fragment");

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(transforms.elements), null, GL_DYNAMIC_DRAW);
	}
	g_window.width = platform_data.window_width;
	g_window.height = platform_data.window_height;
	g_window.size = v2ii(g_window.width, g_window.height);
	g_window.center = v2_mul(g_window.size, 0.5f);

	if(g_connected)
	{
		enet_loop(g_client, 0, game.config);
		if(g_platform_data.quit_after_this_frame)
		{
			enet_peer_disconnect(server, 0);
			enet_loop(g_client, 1000, game.config);
		}
	}

	game.update_timer += g_platform_data.time_passed;
	while(game.update_timer >= c_update_delay)
	{
		game.update_timer -= c_update_delay;
		memcpy(e.prev_x, e.x, sizeof(e.x));
		memcpy(e.prev_y, e.y, sizeof(e.y));
		update(game.config);

		for(int k_i = 0; k_i < c_max_keys; k_i++)
		{
			g_input->keys[k_i].count = 0;
		}
		char_event_arr->count = 0;
	}

	float interpolation_dt = (float)(game.update_timer / c_update_delay);
	render(interpolation_dt);
	memset(e.drawn_last_render, true, sizeof(e.drawn_last_render));

	frame_arena.used = 0;

	total_time += (float)platform_data.time_passed;

	if(g_platform_data.quit_after_this_frame)
	{
		game.config.player_name = main_menu.player_name;
		save_config(game.config);
	}

}

func void update(s_config config)
{

	switch(state)
	{
		case e_state_main_menu:
		{
			if(config.player_name.len >= 3)
			{
				main_menu.player_name = config.player_name;
				state = e_state_game;
				connect_to_server(config);
				break;
			}
			while(true)
			{
				s_char_event event = get_char_event();
				int c = event.c;
				if(!c) { break; }

				if(event.is_symbol)
				{
					if(c == key_backspace)
					{
						if(main_menu.player_name.len > 0)
						{
							main_menu.player_name.data[--main_menu.player_name.len] = 0;
						}
					}
					else if(c == key_enter)
					{
						if(main_menu.player_name.len < 3)
						{
							main_menu.error_str = "Character name needs to be at least 3 characters";
						}
						else
						{
							main_menu.error_str = null;
							state = e_state_game;
							connect_to_server(config);
							break;
						}
					}
				}
				else
				{
					if(c >= 32 && c <= 126)
					{
						if(main_menu.player_name.len < max_player_name_length)
						{
							main_menu.player_name.data[main_menu.player_name.len++] = (char)c;
						}
					}
				}
			}
		} break;

		case e_state_game:
		{

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		cheats, for testing start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			#ifdef m_debug
			if(is_key_pressed(key_add))
			{
				send_simple_packet(server, e_packet_cheat_next_level, ENET_PACKET_FLAG_RELIABLE);
			}
			if(is_key_pressed(key_subtract))
			{
				send_simple_packet(server, e_packet_cheat_previous_level, ENET_PACKET_FLAG_RELIABLE);
			}
			#endif // m_debug
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		cheats, for testing end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			for(int i = 0; i < c_num_threads; i++)
			{
				gravity_system(i * c_entities_per_thread, c_entities_per_thread);
			}
			for(int i = 0; i < c_num_threads; i++)
			{
				input_system(i * c_entities_per_thread, c_entities_per_thread);
			}
			for(int i = 0; i < c_num_threads; i++)
			{
				move_system(i * c_entities_per_thread, c_entities_per_thread);
				player_movement_system(i * c_entities_per_thread, c_entities_per_thread);
				physics_movement_system(i * c_entities_per_thread, c_entities_per_thread);
			}
			for(int i = 0; i < c_num_threads; i++)
			{
				player_bounds_check_system(i * c_entities_per_thread, c_entities_per_thread);
				projectile_bounds_check_system(i * c_entities_per_thread, c_entities_per_thread);
			}
			for(int i = 0; i < c_num_threads; i++)
			{
				projectile_spawner_system(i * c_entities_per_thread, c_entities_per_thread);
			}
			for(int i = 0; i < c_num_threads; i++)
			{
				collision_system(i * c_entities_per_thread, c_entities_per_thread);
			}
			for(int i = 0; i < c_num_threads; i++)
			{
				increase_time_lived_system(i * c_entities_per_thread, c_entities_per_thread);
			}

			int my_player = find_player_by_id(my_id);
			if(my_player != c_invalid_entity)
			{
				s_player_update_from_client data = zero;
				data.x = e.x[my_player];
				data.y = e.y[my_player];
				send_packet(server, e_packet_player_update, data, 0);
			}

			level_timer += delta;
			spawn_system(levels[current_level]);

		} break;
	}
}

func void render(float dt)
{
	switch(state)
	{
		case e_state_main_menu:
		{
			s_v2 pos = g_window.center;
			pos.y -= 100;
			draw_text("Enter character name", pos, 0, v41f(1), e_font_medium, true, zero);
			pos.y += 100;
			if(main_menu.player_name.len > 0)
			{
				draw_text(main_menu.player_name.data, pos, 0, v41f(1), e_font_medium, true, zero);
				pos.y += 100;
			}

			if(main_menu.error_str)
			{
				draw_text(main_menu.error_str, pos, 0, v4(1, 0, 0, 1), e_font_medium, true, zero);
				pos.y += 100;
			}
		} break;

		case e_state_game:
		{
			for(int i = 0; i < c_num_threads; i++)
			{
				draw_system(i * c_entities_per_thread, c_entities_per_thread, dt);
				draw_circle_system(i * c_entities_per_thread, c_entities_per_thread, dt);
			}

			// @Note(tkap, 23/06/2023): Display how many seconds left to beat the level
			{
				float seconds_left = c_level_duration - level_timer;
				s_v2 pos = v2(
					g_window.center.x,
					g_window.size.y * 0.3f
				);
				draw_text(format_text("%i", (int)ceilf(seconds_left)), pos, 1, v41f(1), e_font_medium, true, zero);
			}

			// @Note(tkap, 23/06/2023): Display current level
			{
				s_v2 pos = v2(20, 20);
				draw_text(format_text("Level %i", current_level + 1), pos, 1, v41f(1), e_font_medium, false, zero);
			}

			// @Note(tkap, 25/06/2023): Display time alive of each player
			{
				struct s_player_and_time
				{
					int index;
					float time;

					bool operator>(s_player_and_time right)
					{
						return time < right.time;
					}

				};
				s_sarray<s_player_and_time, 64> player_and_time_arr;
				for(int i = 0; i < c_max_entities; i++)
				{
					if(!e.active[i]) { continue; }
					if(!e.player_id[i]) { continue; }

					s_player_and_time pat = zero;
					pat.index = i;
					pat.time = e.time_lived[i];
					player_and_time_arr.add(pat);
				}
				player_and_time_arr.bubble_sort();

				s_v2 pos = v2(20, 60);
				for(int pat_i = 0; pat_i < player_and_time_arr.count; pat_i++)
				{
					s_player_and_time pat = player_and_time_arr[pat_i];
					char* text = format_text("%s: %i", e.name[pat.index].data, roundfi(pat.time));
					draw_text(text, pos, 1, v41f(1), e_font_medium, false, zero);
					pos.y += g_font_arr[e_font_medium].size;
				}
			}


		} break;
	}

	draw_rect(g_window.center, 0, g_window.size, v41f(1), {.do_background = true});

	{
		glUseProgram(g_program);
		// glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, g_window.width, g_window.height);
		// glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_GREATER);

		{
			int location = glGetUniformLocation(g_program, "window_size");
			glUniform2fv(location, 1, &g_window.size.x);
		}
		{
			int location = glGetUniformLocation(g_program, "time");
			glUniform1f(location, total_time);
		}

		if(transforms.count > 0)
		{
			glEnable(GL_BLEND);
			// glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glBlendFunc(GL_ONE, GL_ONE);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*transforms.elements) * transforms.count, transforms.elements);
			glDrawArraysInstanced(GL_TRIANGLES, 0, 6, transforms.count);
			transforms.count = 0;
		}

		for(int font_i = 0; font_i < e_font_count; font_i++)
		{
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			if(text_arr[font_i].count > 0)
			{
				s_font* font = &g_font_arr[font_i];
				glBindTexture(GL_TEXTURE_2D, font->texture.id);
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*text_arr[font_i].elements) * text_arr[font_i].count, text_arr[font_i].elements);
				glDrawArraysInstanced(GL_TRIANGLES, 0, 6, text_arr[font_i].count);
				text_arr[font_i].count = 0;
			}
		}
	}

	#ifdef _WIN32
	#ifdef m_debug
	hot_reload_shaders();
	#endif // m_debug
	#endif // _WIN32
}

func b8 check_for_shader_errors(u32 id, char* out_error)
{
	int compile_success;
	char info_log[1024];
	glGetShaderiv(id, GL_COMPILE_STATUS, &compile_success);

	if(!compile_success)
	{
		glGetShaderInfoLog(id, 1024, null, info_log);
		log("Failed to compile shader:\n%s", info_log);

		if(out_error)
		{
			strcpy(out_error, info_log);
		}

		return false;
	}
	return true;
}

func void input_system(int start, int count)
{
	b8 go_left = is_key_down(key_a) || is_key_down(key_left);
	b8 go_right = is_key_down(key_d) || is_key_down(key_right);
	b8 go_down = is_key_pressed(key_s) || is_key_pressed(key_down);
	b8 jump = is_key_pressed(key_space) || is_key_pressed(key_w) || is_key_pressed(key_up);
	b8 jump_released = is_key_released(key_space) || is_key_released(key_w) || is_key_released(key_up);

	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_input]) { continue; }

		e.dir_x[ii] = 0;
		if(go_right)
		{
			e.dir_x[ii] += 1;
		}
		if(go_left)
		{
			e.dir_x[ii] -= 1;
		}

		if(go_down)
		{
			e.vel_y[ii] = max(e.vel_y[ii], c_fast_fall_speed);
		}

		b8 can_jump = e.jumps_done[ii] < 2;
		if(can_jump && jump)
		{
			if(e.jumps_done[ii] == 0)
			{
				play_sound_if_supported(jump_sound);
			}
			else
			{
				play_sound_if_supported(jump2_sound);
			}
			float jump_multiplier = e.jumps_done[ii] == 0 ? 1.0f : 0.9f;
			e.vel_y[ii] = c_jump_strength * jump_multiplier;
			e.jumping[ii] = true;
			e.jumps_done[ii] += 1;
		}
		else if(e.jumping[ii] && jump_released && e.vel_y[ii] < 0)
		{
			e.vel_y[ii] *= 0.5f;
		}
	}
}

func void draw_system(int start, int count, float dt)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_draw]) { continue; }

		float x = lerp(e.prev_x[ii], e.x[ii], dt);
		float y = lerp(e.prev_y[ii], e.y[ii], dt);

		s_v4 color = e.color[ii];
		if(e.dead[ii])
		{
			color.w = 0.25f;
		}
		draw_rect(v2(x, y), 0, v2(e.sx[ii], e.sy[ii]), color, zero);

		s_v2 pos = v2(
			x, y
		);
		pos.y -= e.sy[ii];

		if(!e.dead[ii])
		{
			if(e.player_id[ii] == my_id)
			{
				draw_text(main_menu.player_name.data, pos, 1, color, e_font_small, true, zero);
			}
			else
			{
				if(e.name[ii].len > 0)
				{
					draw_text(e.name[ii].data, pos, 1, color, e_font_small, true, zero);
				}
			}
		}
	}
}

func void draw_circle_system(int start, int count, float dt)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_draw_circle]) { continue; }

		float x = lerp(e.prev_x[ii], e.x[ii], dt);
		float y = lerp(e.prev_y[ii], e.y[ii], dt);

		s_v4 light_color = e.color[ii];
		light_color.w *= 0.2f;
		draw_light(v2(x, y), 0, e.sx[ii] * 8.0f, light_color, zero);
		draw_circle(v2(x, y), 1, e.sx[ii], e.color[ii], zero);
		draw_circle(v2(x, y), 2, e.sx[ii] * 0.7f, v41f(1), zero);
	}
}

func void parse_packet(ENetEvent event, s_config config)
{
	u8* cursor = event.packet->data;
	e_packet packet_id = *(e_packet*)buffer_read(&cursor, sizeof(packet_id));

	switch(packet_id)
	{
		case e_packet_welcome:
		{
			s_welcome_from_server data = *(s_welcome_from_server*)cursor;
			my_id = data.id;
			int entity = make_player(data.id, true, config.color);
			e.name[entity] = main_menu.player_name;
		} break;

		case e_packet_already_connected_player:
		{
			s_already_connected_player_from_server data = *(s_already_connected_player_from_server*)cursor;
			int entity = make_player(data.id, data.dead, data.color);
			e.name[entity] = data.name;
			e.color[entity] = data.color;
			log("Got already connected data of %u", data.id);
		} break;

		case e_packet_another_player_connected:
		{
			s_another_player_connected_from_server data = *(s_another_player_connected_from_server*)cursor;
			make_player(data.id, data.dead, v41f(1));
		} break;

		case e_packet_player_update:
		{
			s_player_update_from_server data = *(s_player_update_from_server*)cursor;

			assert(data.id != my_id);
			int entity = find_player_by_id(data.id);
			if(entity != c_invalid_entity)
			{
				e.x[entity] = data.x;
				e.y[entity] = data.y;
			}

		} break;

		case e_packet_player_disconnected:
		{
			s_player_disconnected_from_server data = *(s_player_disconnected_from_server*)cursor;
			assert(data.id != my_id);
			int entity = find_player_by_id(data.id);
			if(entity != c_invalid_entity)
			{
				e.active[entity] = false;
			}
		} break;

		case e_packet_beat_level:
		{
			s_beat_level_from_server data = *(s_beat_level_from_server*)cursor;
			current_level = data.current_level + 1;
			rng.seed = data.seed;
			reset_level();
			revive_every_player();
			log("Beat level %i", current_level);
			play_sound_if_supported(win_sound);
		} break;

		case e_packet_reset_level:
		{
			s_reset_level_from_server data = *(s_reset_level_from_server*)cursor;
			current_level = data.current_level;
			log("Reset level %i", current_level + 1);
			rng.seed = data.seed;
			reset_level();
			revive_every_player();
		} break;

		case e_packet_player_got_hit:
		{
			s_player_got_hit_from_server data = *(s_player_got_hit_from_server*)cursor;
			assert(data.id != my_id);
			int entity = find_player_by_id(data.id);
			if(entity != c_invalid_entity)
			{
				log("Player %i with id %u died", entity, data.id);
				e.dead[entity] = true;
			}
		} break;

		case e_packet_player_appearance:
		{
			s_player_appearance_from_server data = *(s_player_appearance_from_server*)cursor;
			assert(data.id != my_id);

			int entity = find_player_by_id(data.id);
			if(entity != c_invalid_entity)
			{
				e.name[entity] = data.name;
				e.color[entity] = data.color;
				log("Set %u's name to %s", data.id, e.name[entity].data);
			}
		} break;

		#ifdef m_debug
		case e_packet_cheat_previous_level:
		{
			s_cheat_previous_level_from_server data = *(s_cheat_previous_level_from_server*)cursor;
			current_level = data.current_level;
			rng.seed = data.seed;
			reset_level();
			revive_every_player();
		} break;
		#endif // m_debug

		case e_packet_all_levels_beat:
		{
			current_level = 0;
			reset_level();
			revive_every_player();
		} break;

		case e_packet_update_time_lived:
		{
			s_update_time_lived_from_server data = *(s_update_time_lived_from_server*)cursor;

			int entity = find_player_by_id(data.id);
			if(entity != c_invalid_entity)
			{
				e.time_lived[entity] = data.time_lived;
			}
		} break;

		invalid_default_case;
	}
}

func void enet_loop(ENetHost* client, int timeout, s_config config)
{
	ENetEvent event = zero;
	while(enet_host_service(client, &event, timeout) > 0)
	{
		switch(event.type)
		{
			case ENET_EVENT_TYPE_NONE:
			{
			} break;

			case ENET_EVENT_TYPE_CONNECT:
			{
				log("Connected!");

				s_player_appearance_from_client data;
				data.name = main_menu.player_name;
				data.color = config.color;
				send_packet(server, e_packet_player_appearance, data, ENET_PACKET_FLAG_RELIABLE);

			} break;

			case ENET_EVENT_TYPE_DISCONNECT:
			{
				log("Disconnected!\n");
				return;
			} break;

			case ENET_EVENT_TYPE_RECEIVE:
			{
				parse_packet(event, config);
				enet_packet_destroy(event.packet);
			} break;

			invalid_default_case;
		}
	}
}

func void revive_every_player(void)
{
	for(int i = 0; i < c_max_entities; i++)
	{
		if(!e.active[i]) { continue; }
		if(!e.player_id[i]) { continue; }
		e.dead[i] = false;
		log("Revived player at index %i with id %u", i, e.player_id[i]);
	}
}

func void collision_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_collide]) { continue; }

		for(int j = 0; j < c_max_entities; j++)
		{
			if(!e.active[j]) { continue; }
			if(e.dead[j]) { continue; }
			if(e.type[j] != e_entity_type_player) { continue; }
			if(e.player_id[j] != my_id) { continue; }

			if(
				rect_collides_circle(v2(e.x[j], e.y[j]), v2(e.sx[j], e.sy[j]), v2(e.x[ii], e.y[ii]), e.sx[ii] * 0.48f)
			)
			{
				e.dead[j] = true;
				s_player_got_hit_from_client data = zero;
				send_packet(server, e_packet_player_got_hit, data, ENET_PACKET_FLAG_RELIABLE);
			}
		}
	}
}

func s_font load_font(char* path, float font_size, s_lin_arena* arena)
{
	s_font font = zero;
	font.size = font_size;

	u8* file_data = (u8*)read_file(path, arena);
	assert(file_data);

	stbtt_fontinfo info = zero;
	stbtt_InitFont(&info, file_data, 0);

	stbtt_GetFontVMetrics(&info, &font.ascent, &font.descent, &font.line_gap);

	font.scale = stbtt_ScaleForPixelHeight(&info, font_size);
	#define max_chars 128
	int bitmap_count = 0;
	u8* bitmap_arr[max_chars];
	const int padding = 10;
	int total_width = padding;
	int total_height = 0;
	for(int char_i = 0; char_i < max_chars; char_i++)
	{
		s_glyph glyph = zero;
		u8* bitmap = stbtt_GetCodepointBitmap(&info, 0, font.scale, char_i, &glyph.width, &glyph.height, 0, 0);
		stbtt_GetCodepointBox(&info, char_i, &glyph.x0, &glyph.y0, &glyph.x1, &glyph.y1);
		stbtt_GetGlyphHMetrics(&info, char_i, &glyph.advance_width, null);

		total_width += glyph.width + padding;
		total_height = max(glyph.height + padding * 2, total_height);

		font.glyph_arr[char_i] = glyph;
		bitmap_arr[bitmap_count++] = bitmap;
	}

	// @Fixme(tkap, 23/06/2023): Use arena
	u8* gl_bitmap = (u8*)calloc(1, sizeof(u8) * 4 * total_width * total_height);

	int current_x = padding;
	for(int char_i = 0; char_i < max_chars; char_i++)
	{
		s_glyph* glyph = &font.glyph_arr[char_i];
		u8* bitmap = bitmap_arr[char_i];
		for(int y = 0; y < glyph->height; y++)
		{
			for(int x = 0; x < glyph->width; x++)
			{
				u8 src_pixel = bitmap[x + y * glyph->width];
				u8* dst_pixel = &gl_bitmap[((current_x + x) + (padding + y) * total_width) * 4];
				dst_pixel[0] = src_pixel;
				dst_pixel[1] = src_pixel;
				dst_pixel[2] = src_pixel;
				dst_pixel[3] = src_pixel;
			}
		}

		glyph->uv_min.x = current_x / (float)total_width;
		glyph->uv_max.x = (current_x + glyph->width) / (float)total_width;

		glyph->uv_min.y = padding / (float)total_height;

		// @Note(tkap, 17/05/2023): For some reason uv_max.y is off by 1 pixel (checked the texture in renderoc), which causes the text to be slightly miss-positioned
		// in the Y axis. "glyph->height - 1" fixes it.
		glyph->uv_max.y = (padding + glyph->height - 1) / (float)total_height;

		// @Note(tkap, 17/05/2023): Otherwise the line above makes the text be cut off at the bottom by 1 pixel...
		glyph->uv_max.y += 0.01f;

		current_x += glyph->width + padding;
	}

	font.texture = load_texture_from_data(gl_bitmap, total_width, total_height, GL_LINEAR);

	for(int bitmap_i = 0; bitmap_i < bitmap_count; bitmap_i++)
	{
		stbtt_FreeBitmap(bitmap_arr[bitmap_i], null);
	}

	free(gl_bitmap);

	#undef max_chars

	return font;
}

func s_texture load_texture_from_data(void* data, int width, int height, u32 filtering)
{
	assert(data);
	u32 id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering);

	s_texture texture = zero;
	texture.id = id;
	texture.size = v22i(width, height);
	return texture;
}

func s_v2 get_text_size_with_count(char* text, e_font font_id, int count)
{
	assert(count >= 0);
	if(count <= 0) { return zero; }
	s_font* font = &g_font_arr[font_id];

	s_v2 size = zero;
	size.y = font->size;

	for(int char_i = 0; char_i < count; char_i++)
	{
		char c = text[char_i];
		s_glyph glyph = font->glyph_arr[c];
		if(char_i == count - 1 && c != ' ')
		{
			size.x += glyph.width;
		}
		else
		{
			size.x += glyph.advance_width * font->scale;
		}
	}

	return size;
}

func s_v2 get_text_size(char* text, e_font font_id)
{
	return get_text_size_with_count(text, font_id, (int)strlen(text));
}

func void connect_to_server(s_config config)
{
	if(enet_initialize() != 0)
	{
		error(false);
	}

	g_client = enet_host_create(
		null /* create a client host */,
		1, /* only allow 1 outgoing connection */
		2, /* allow up 2 channels to be used, 0 and 1 */
		0, /* assume any amount of incoming bandwidth */
		0 /* assume any amount of outgoing bandwidth */
	);

	if(g_client == null)
	{
		error(false);
	}

	ENetAddress address = zero;
	enet_address_set_host(&address, config.ip.data);
	// enet_address_set_host(&address, "127.0.0.1");
	address.port = (u16)config.port;

	server = enet_host_connect(g_client, &address, 2, 0);
	if(server == null)
	{
		error(false);
	}

	g_connected = true;

}

#ifdef _WIN32
#ifdef m_debug
global FILETIME last_write_time = zero;
func void hot_reload_shaders(void)
{
	WIN32_FIND_DATAA find_data = zero;
	HANDLE handle = FindFirstFileA("shaders/fragment.fragment", &find_data);
	if(handle == INVALID_HANDLE_VALUE) { return; }

	if(CompareFileTime(&last_write_time, &find_data.ftLastWriteTime) == -1)
	{
		// @Note(tkap, 23/06/2023): This can fail because text editor may be locking the file, so we check if it worked
		u32 new_program = load_shader("shaders/vertex.vertex", "shaders/fragment.fragment");
		if(new_program)
		{
			if(g_program)
			{
				glUseProgram(0);
				glDeleteProgram(g_program);
			}
			g_program = load_shader("shaders/vertex.vertex", "shaders/fragment.fragment");
			last_write_time = find_data.ftLastWriteTime;
		}
	}

	FindClose(handle);

}
#endif // m_debug
#endif // _WIN32

func u32 load_shader(char* vertex_path, char* fragment_path)
{
	u32 vertex = glCreateShader(GL_VERTEX_SHADER);
	u32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
	char* header = "#version 430 core\n";
	char* vertex_src = read_file(vertex_path, &frame_arena);
	if(!vertex_src || !vertex_src[0]) { return 0; }
	char* fragment_src = read_file(fragment_path, &frame_arena);
	if(!fragment_src || !fragment_src[0]) { return 0; }
	char* vertex_src_arr[] = {header, read_file("src/shader_shared.h", &frame_arena), vertex_src};
	char* fragment_src_arr[] = {header, read_file("src/shader_shared.h", &frame_arena), fragment_src};
	glShaderSource(vertex, array_count(vertex_src_arr), vertex_src_arr, null);
	glShaderSource(fragment, array_count(fragment_src_arr), fragment_src_arr, null);
	glCompileShader(vertex);
	char buffer[1024] = zero;
	check_for_shader_errors(vertex, buffer);
	glCompileShader(fragment);
	check_for_shader_errors(fragment, buffer);
	u32 program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	return program;
}

func void handle_instant_movement_(int entity)
{
	assert(entity != c_invalid_entity);
	e.prev_x[entity] = e.x[entity];
	e.prev_y[entity] = e.y[entity];
}

func s_config read_config_or_make_default(s_lin_arena* arena, s_rng* in_rng)
{
	s_config config = zero;

	char* data = read_file("config.txt", arena);
	if(!data)
	{
		return make_default_config(in_rng);
	}

	struct s_query_data
	{
		char* query;
		void* target;
		int type;
	};

	s_query_data queries[] =
	{
		{.query = "name=", .target = &config.player_name, .type = 0},
		{.query = "ip=", .target = &config.ip, .type = 0},
		{.query = "port=", .target = &config.port, .type = 1},
		{.query = "color=", .target = &config.color, .type = 2},
	};

	for(int query_i = 0; query_i < array_count(queries); query_i++)
	{
		s_query_data query = queries[query_i];
		char* where = strstr(data, query.query);
		if(!where)
		{
			log("Malformed config file. Generating default config");
			return make_default_config(in_rng);
		}
		char* start = where + strlen(query.query);
		char* cursor = start;
		while(true)
		{
			b8 do_thing = false;
			if(*cursor == 0)
			{
				do_thing = true;
			}
			else if(*cursor == '\n' || *cursor == '\r')
			{
				do_thing = true;
			}
			else
			{
				cursor += 1;
			}
			if(do_thing)
			{
				if(cursor - start <= 0)
				{
					log("Malformed config file. Generating default config");
					return make_default_config(in_rng);
				}

				// @Note(tkap, 24/06/2023): string
				if(query.type == 0)
				{
					s_name* name = (s_name*)query.target;
					memcpy(name->data, start, cursor - start);
					name->len = (int)(cursor - start);
					break;
				}

				// @Note(tkap, 24/06/2023): int
				else if(query.type == 1)
				{
					char buffer[32] = zero;
					memcpy(buffer, start, cursor - start);
					*(int*)query.target = atoi(buffer);
					break;
				}

				// @Note(tkap, 24/06/2023): color
				else if(query.type == 2)
				{
					char buffer[32] = zero;
					memcpy(buffer, start, cursor - start);
					int val = (int)strtol(buffer, null, 16);
					s_v4* color = (s_v4*)query.target;
					float r = ((val & 0xFF0000) >> 16) / 255.0f;
					float g = ((val & 0x00FF00) >> 8) / 255.0f;
					float b = ((val & 0x0000FF) >> 0) / 255.0f;
					color->x = r;
					color->y = g;
					color->z = b;
					color->w = 1;
					break;
				}
			}
		}
		data = cursor;
	}

	return config;
}

func s_config make_default_config(s_rng* in_rng)
{
	s_config config = zero;
	config.ip = make_name("at-taxation.at.ply.gg");
	config.port = 62555;
	config.color.x = (randu(in_rng) % 256) / 255.0f;
	config.color.y = (randu(in_rng) % 256) / 255.0f;
	config.color.z = (randu(in_rng) % 256) / 255.0f;
	config.color.w = 1;
	return config;
}

func void save_config(s_config config)
{
	s_str_builder builder = zero;
	builder_add_line(&builder, "name=%s", config.player_name.data);
	builder_add_line(&builder, "ip=%s", config.ip.data);
	builder_add_line(&builder, "port=%i", config.port);

	int r = roundfi(config.color.x * 255);
	int g = roundfi(config.color.y * 255);
	int b = roundfi(config.color.z * 255);
	builder_add(&builder, "color=%02x%02x%02x", r, g, b);
	b8 result = write_file("config.txt", builder.data, builder.len);
	if(!result)
	{
		log("Failed to write config.txt");
	}
}

func s_name make_name(char* str)
{
	s_name result = zero;
	int len = (int)strlen(str);
	assert(len < max_player_name_length);
	memcpy(result.data, str, len);
	result.len = len;
	return result;
}

func b8 is_key_down(int key)
{
	assert(key < c_max_keys);
	return g_input->keys[key].is_down || g_input->keys[key].count >= 2;
}

func b8 is_key_up(int key)
{
	assert(key < c_max_keys);
	return !g_input->keys[key].is_down;
}

func b8 is_key_pressed(int key)
{
	assert(key < c_max_keys);
	return (g_input->keys[key].is_down && g_input->keys[key].count == 1) || g_input->keys[key].count > 1;
}

func b8 is_key_released(int key)
{
	assert(key < c_max_keys);
	return (!g_input->keys[key].is_down && g_input->keys[key].count == 1) || g_input->keys[key].count > 1;
}

func s_char_event get_char_event()
{
	s_char_event event = zero;
	if(char_event_arr->count > 0)
	{
		event = char_event_arr->elements[0];
		char_event_arr->remove_and_shift(0);
	}
	return event;
}

void gl_debug_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	unreferenced(userParam);
	unreferenced(length);
	unreferenced(id);
	unreferenced(type);
	unreferenced(source);
	if(severity >= GL_DEBUG_SEVERITY_HIGH)
	{
		printf("GL ERROR: %s\n", message);
		assert(false);
	}
}
