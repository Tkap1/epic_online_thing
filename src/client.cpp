#define m_client 1
static constexpr int ENET_PACKET_FLAG_RELIABLE = 1;

#ifdef _WIN32
// @Note(tkap, 24/06/2023): We don't want this Madeg
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <GL/gl.h>
#include "external/glcorearb.h"
#include "external/wglext.h"
#include <stdlib.h>
#define EPIC_DLLEXPORT __declspec(dllexport)
#else
#include<X11/X.h>
#include<X11/Xlib.h>
#include<GL/gl.h>
#include<GL/glx.h>
#include<GL/glext.h>
#include <x86intrin.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#define EPIC_DLLEXPORT
#endif // _WIN32

#include <stdio.h>
#include <math.h>
#include "types.h"
#include "utils.h"
#include "epic_math.h"
#include "config.h"
#include "memory.h"
#include "shared_all.h"
#include "shared_client_server.h"
#include "platform_shared_client.h"
#include "shared.h"
#include "rng.h"
#include "client.h"
#include "shader_shared.h"
#include "str_builder.h"
#include "audio.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert assert
#include "external/stb_truetype.h"

global s_sarray<s_transform, c_max_entities> transforms;
global s_sarray<s_transform, c_max_entities> text_arr[e_font_count];

global s_lin_arena* frame_arena;

global s_game_window g_window;
global s_input* g_input;
global s_sarray<s_char_event, 1024>* char_event_arr;

global s_platform_data g_platform_data;
global s_platform_funcs g_platform_funcs;
global s_game_network* g_network;

global s_game* game;

#define X(type, name) type name = null;
m_gl_funcs
#undef X

#include "draw.cpp"
#include "memory.cpp"
#include "file.cpp"
#include "shared.cpp"
#include "str_builder.cpp"
#include "audio.cpp"

global b8 playback;
global int frame;
global FILE* state_file;

extern "C"
{
EPIC_DLLEXPORT
m_update_game(update_game)
{
	static_assert(c_game_memory >= sizeof(s_game));
	static_assert((c_max_entities % c_num_threads) == 0);
	game = (s_game*)game_memory;
	frame_arena = &platform_data.frame_arena;
	g_platform_funcs = platform_funcs;
	g_platform_data = platform_data;
	g_network = game_network;
	char_event_arr = platform_data.char_event_arr;
	g_input = platform_data.input;
	if(disgusting_recompile_hack) { return ; }

	if(!game->initialized)
	{

		remove("stored_state");

		game->initialized = true;
		#define X(type, name) name = (type)platform_funcs.load_gl_func(#name);
		m_gl_funcs
		#undef X

		glDebugMessageCallback(gl_debug_callback, null);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		game->rng.seed = (u32)__rdtsc();

		game->config = read_config_or_make_default(frame_arena, &game->rng);

		platform_funcs.set_swap_interval(1);

		game->jump_sound = load_wav("assets/jump.wav", frame_arena);
		game->jump2_sound = load_wav("assets/jump2.wav", frame_arena);
		game->big_dog_sound = load_wav("assets/big_dog.wav", frame_arena);
		game->win_sound = load_wav("assets/win.wav", frame_arena);

		game->font_arr[e_font_small] = load_font("assets/consola.ttf", 24, frame_arena);
		game->font_arr[e_font_medium] = load_font("assets/consola.ttf", 36, frame_arena);
		game->font_arr[e_font_big] = load_font("assets/consola.ttf", 72, frame_arena);

		u32 vao;
		u32 ssbo;
		game->program = load_shader("shaders/vertex.vertex", "shaders/fragment.fragment");

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		glGenBuffers(1, &ssbo);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(transforms.elements), null, GL_DYNAMIC_DRAW);
	}

	if(platform_data.recompiled)
	{
		#define X(type, name) name = (type)platform_funcs.load_gl_func(#name);
		m_gl_funcs
		#undef X
		init_levels();
	}


	g_window.width = platform_data.window_width;
	g_window.height = platform_data.window_height;
	g_window.size = v2ii(g_window.width, g_window.height);
	g_window.center = v2_mul(g_window.size, 0.5f);

	b8 f1 = is_key_pressed(c_key_f1);
	b8 left = is_key_down(c_key_left);
	b8 right = is_key_down(c_key_right);

	game->update_timer += g_platform_data.time_passed;
	while(game->update_timer >= c_update_delay)
	{
		game->update_timer -= c_update_delay;
		memcpy(game->e.prev_x, game->e.x, sizeof(game->e.x));
		memcpy(game->e.prev_y, game->e.y, sizeof(game->e.y));
		memcpy(game->e.prev_sx, game->e.sx, sizeof(game->e.sx));
		memcpy(game->e.prev_sy, game->e.sy, sizeof(game->e.sy));
		update(game->config);

		for(int k_i = 0; k_i < c_max_keys; k_i++)
		{
			g_input->keys[k_i].count = 0;
		}
		char_event_arr->count = 0;

		if(!playback)
		{
			save_game_state(game);
		}
	}


	if(f1)
	{
		playback = !playback;
	}
	if(playback)
	{
		if(left)
		{
			frame = at_least(0, frame - 1);
		}
		if(right)
		{
			// frame = at_most(??, frame + 1);
			frame++;
		}
		printf("%i\n", frame);

		if(state_file)
		{
			fclose(state_file);
			state_file = null;
		}
		load_game(game);
	}


	float interpolation_dt = (float)(game->update_timer / c_update_delay);
	render(interpolation_dt);
	memset(game->e.drawn_last_render, true, sizeof(game->e.drawn_last_render));

	game->total_time += (float)platform_data.time_passed;

	if(g_platform_data.quit_after_this_frame)
	{
		game->config.player_name = game->main_menu.player_name;
		g_network->disconnect = true;
		save_config(game->config);
	}

	frame_arena->used = 0;

}
}

func void update(s_config config)
{
	switch(game->state)
	{

		case e_state_main_menu:
		{
			if(config.player_name.len >= 3)
			{
				game->main_menu.player_name = config.player_name;
				game->state = e_state_game;
				connect_to_server(config);
				break;
			}

			e_string_input_result string_input_result = handle_string_input(&game->main_menu.player_name);
			if(string_input_result == e_string_input_result_submit)
			{
				if(game->main_menu.player_name.len < 3)
				{
					game->main_menu.error_str = "Character name needs to be at least 3 characters";
				}
				else
				{
					game->main_menu.error_str = null;
					game->state = e_state_game;
					connect_to_server(config);
					break;
				}
			}
		} break;

		case e_state_game:
		{

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		cheats, for testing start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			#ifdef m_debug
			if(is_key_pressed(c_key_add))
			{
				send_simple_packet(e_packet_cheat_next_level, ENET_PACKET_FLAG_RELIABLE);
			}
			if(is_key_pressed(c_key_subtract))
			{
				send_simple_packet(e_packet_cheat_previous_level, ENET_PACKET_FLAG_RELIABLE);
			}
			#endif // m_debug
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		cheats, for testing end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			for(int i = 0; i < c_num_threads; i++)
			{
				increase_time_lived_system(i * c_entities_per_thread, c_entities_per_thread);
			}
			for(int i = 0; i < c_num_threads; i++)
			{
				modify_speed_system(i * c_entities_per_thread, c_entities_per_thread);
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

			int my_player = find_player_by_id(game->my_id);
			if(my_player != c_invalid_entity)
			{
				s_player_update_from_client data = zero;
				data.x = game->e.x[my_player];
				data.y = game->e.y[my_player];
				send_packet(e_packet_player_update, data, 0);
			}

			level_timer += delta;
			spawn_system(levels[game->current_level]);

			if(game->chatting)
			{
				e_string_input_result input_result = handle_string_input(&game->my_chat_msg);
				if(input_result == e_string_input_result_submit)
				{
					if(game->my_chat_msg.len > 0)
					{
						s_chat_msg_from_client data;
						data.msg = game->my_chat_msg;
						send_packet(e_packet_chat_msg, data, ENET_PACKET_FLAG_RELIABLE);
						game->my_chat_msg.len = 0;
					}
					game->chatting = false;
				}
				if(is_key_pressed(c_key_escape))
				{
					game->my_chat_msg.len = 0;
					game->chatting = false;
				}
			}
			else
			{
				if(is_key_pressed(c_key_enter))
				{
					game->chatting = true;
				}
			}

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		chat messages start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{
				for(int msg_i = 0; msg_i < c_max_peers; msg_i++)
				{
					if(game->chat_message_ids[msg_i])
					{
						game->chat_message_times[msg_i] += delta;
						if(game->chat_message_times[msg_i] >= 5)
						{
							game->chat_message_ids[msg_i] = 0;
						}
					}
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		chat messages end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		} break;

		case e_state_count:
		{
			assert(0);
		} break;
	}
}

func void render(float dt)
{
	switch(game->state)
	{
		case e_state_main_menu:
		{
			s_v2 pos = g_window.center;
			pos.y -= 100;
			draw_text("Enter character name", pos, 0, v41f(1), e_font_medium, true, zero);
			pos.y += 100;
			if(game->main_menu.player_name.len > 0)
			{
				draw_text(game->main_menu.player_name.data, pos, 0, v41f(1), e_font_medium, true, zero);
				pos.y += 100;
			}

			if(game->main_menu.error_str)
			{
				draw_text(game->main_menu.error_str, pos, 0, v4(1, 0, 0, 1), e_font_medium, true, zero);
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
				float seconds_left = levels[game->current_level].duration - level_timer;
				s_v2 pos = v2(
					g_window.center.x,
					g_window.size.y * 0.3f
				);
				draw_text(format_text("%i", (int)ceilf(seconds_left)), pos, 1, v41f(1), e_font_medium, true, zero);
			}

			// @Note(tkap, 27/06/2023): Display if level has infinite jumps
			if(levels[game->current_level].infinite_jumps)
			{
				s_v2 pos = v2(
					g_window.center.x,
					g_window.size.y * 0.35f
				);
				draw_text("Infinite jumps!", pos, 1, v41f(1), e_font_small, true, zero);
			}

			// @Note(tkap, 23/06/2023): Display current level
			{
				s_v2 pos = v2(20, 20);
				draw_text(format_text("Level %i (%i)", game->current_level + 1, game->attempt_count_on_current_level), pos, 1, v41f(1), e_font_medium, false, zero);
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
					if(!game->e.active[i]) { continue; }
					if(!game->e.player_id[i]) { continue; }

					s_player_and_time pat = zero;
					pat.index = i;
					pat.time = game->e.time_lived[i];
					player_and_time_arr.add(pat);
				}
				player_and_time_arr.small_sort();

				s_v2 pos = v2(20, 60);
				for(int pat_i = 0; pat_i < player_and_time_arr.count; pat_i++)
				{
					s_player_and_time pat = player_and_time_arr[pat_i];
					char* text = format_text("%s: %i", game->e.name[pat.index].data, roundfi(pat.time));
					draw_text(text, pos, 1, v41f(1), e_font_medium, false, zero);
					pos.y += game->font_arr[e_font_medium].size;
				}
			}

			if(game->chatting && game->my_id && game->my_chat_msg.len > 0)
			{
				int entity = find_player_by_id(game->my_id);
				if(entity != c_invalid_entity)
				{
					s_v2 pos = v2(
						game->e.x[entity],
						game->e.y[entity] - game->e.sy[entity] * 2
					);
					draw_text(game->my_chat_msg.data, pos, 1, v41f(1), e_font_small, true, zero);
				}
			}

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		render chat messages start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{
				for(int msg_i = 0; msg_i < c_max_peers; msg_i++)
				{
					if(game->chat_message_ids[msg_i] && !(game->chat_message_ids[msg_i] == game->my_id && game->chatting))
					{
						int entity = find_player_by_id(game->chat_message_ids[msg_i]);
						if(entity != c_invalid_entity)
						{
							s_v2 pos = v2(
								game->e.x[entity],
								game->e.y[entity] - game->e.sy[entity] * 2
							);
							draw_text(game->chat_messages[msg_i].data, pos, 1, v41f(1), e_font_small, true, zero);
						}
					}
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		render chat messages end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		} break;

		case e_state_count:
		{
			assert(0);
		} break;
	}

	draw_rect(g_window.center, 0, g_window.size, v41f(1), {.do_background = true});

	{
		glUseProgram(game->program);
		// glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, g_window.width, g_window.height);
		// glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_GREATER);

		{
			int location = glGetUniformLocation(game->program, "window_size");
			glUniform2fv(location, 1, &g_window.size.x);
		}
		{
			int location = glGetUniformLocation(game->program, "time");
			glUniform1f(location, game->total_time);
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
				s_font* font = &game->font_arr[font_i];
				glBindTexture(GL_TEXTURE_2D, font->texture.id);
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*text_arr[font_i].elements) * text_arr[font_i].count, text_arr[font_i].elements);
				glDrawArraysInstanced(GL_TRIANGLES, 0, 6, text_arr[font_i].count);
				text_arr[font_i].count = 0;
			}
		}
	}

	#ifdef m_debug
	hot_reload_shaders();
	#endif // m_debug
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
	b8 go_left = (is_key_down(c_key_a) || is_key_down(c_key_left)) && !game->chatting;
	b8 go_right = (is_key_down(c_key_d) || is_key_down(c_key_right)) && !game->chatting;
	b8 go_down = (is_key_pressed(c_key_s) || is_key_pressed(c_key_down)) && !game->chatting;
	b8 jump = (is_key_pressed(c_key_space) || is_key_pressed(c_key_w) || is_key_pressed(c_key_up)) && !game->chatting;
	b8 jump_released = is_key_released(c_key_space) || is_key_released(c_key_w) || is_key_released(c_key_up);

	s_level level = levels[game->current_level];

	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_input]) { continue; }

		game->e.dir_x[ii] = 0;
		if(go_right)
		{
			game->e.dir_x[ii] += 1;
		}
		if(go_left)
		{
			game->e.dir_x[ii] -= 1;
		}

		if(go_down)
		{
			game->e.vel_y[ii] = max(game->e.vel_y[ii], c_fast_fall_speed);
		}

		b8 can_jump = game->e.jumps_done[ii] < 2;
		if(level.infinite_jumps)
		{
			game->e.jumps_done[ii] = 1;
			can_jump = true;
		}
		if(can_jump && jump)
		{
			if(game->e.jumps_done[ii] == 0)
			{
				play_sound_if_supported(game->jump_sound);
			}
			else
			{
				play_sound_if_supported(game->jump2_sound);
			}
			float jump_multiplier = game->e.jumps_done[ii] == 0 ? 1.0f : 0.9f;
			game->e.vel_y[ii] = c_jump_strength * jump_multiplier;
			game->e.jumping[ii] = true;
			game->e.jumps_done[ii] += 1;
		}
		else if(game->e.jumping[ii] && jump_released && game->e.vel_y[ii] < 0)
		{
			game->e.vel_y[ii] *= 0.5f;
		}
	}
}

func void draw_system(int start, int count, float dt)
{
	b8 am_i_dead = true;
	int my_character = find_player_by_id(game->my_id);
	if(!game->e.dead[my_character])
	{
		am_i_dead = false;
	}

	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_draw]) { continue; }

		float x = lerp(game->e.prev_x[ii], game->e.x[ii], dt);
		float y = lerp(game->e.prev_y[ii], game->e.y[ii], dt);

		float sx = lerp(game->e.prev_sx[ii], game->e.sx[ii], dt);
		float sy = lerp(game->e.prev_sy[ii], game->e.sy[ii], dt);

		b8 this_character_is_me = game->e.player_id[ii] == game->my_id;

		s_v4 color = game->e.color[ii];
		if(game->e.dead[ii])
		{
			color.w *= 0.25f;
		}
		if(!am_i_dead && !this_character_is_me)
		{
			color.w *= 0.15f;
		}
		draw_rect(v2(x, y), 0, v2(sx, sy), color, zero);

		s_v2 pos = v2(
			x, y
		);
		pos.y -= sy;

		if(!game->e.dead[ii])
		{
			if(game->e.player_id[ii] == game->my_id)
			{
				draw_text(game->main_menu.player_name.data, pos, 1, color, e_font_small, true, zero);
			}
			else
			{
				if(game->e.name[ii].len > 0)
				{
					draw_text(game->e.name[ii].data, pos, 1, color, e_font_small, true, zero);
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
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_draw_circle]) { continue; }

		float x = lerp(game->e.prev_x[ii], game->e.x[ii], dt);
		float y = lerp(game->e.prev_y[ii], game->e.y[ii], dt);
		float radius = lerp(game->e.prev_sx[ii], game->e.sx[ii], dt);

		s_v4 light_color = game->e.color[ii];
		light_color.w *= 0.2f;
		draw_light(v2(x, y), 0, radius * 8.0f, light_color, zero);
		draw_circle(v2(x, y), 1, radius, game->e.color[ii], zero);
		draw_circle(v2(x, y), 2, radius * 0.7f, v41f(1), zero);
	}
}

extern "C"
{
EPIC_DLLEXPORT
m_parse_packet(parse_packet)
{
	u8* cursor = packet.data;
	e_packet packet_id = *(e_packet*)buffer_read(&cursor, sizeof(packet_id));

	switch(packet_id)
	{

		case e_packet_connect:
		{
			log("Connected!");
			s_player_appearance_from_client data;
			data.name = game->main_menu.player_name;
			data.color = game->config.color;
			send_packet(e_packet_player_appearance, data, ENET_PACKET_FLAG_RELIABLE);
		} break;

		case e_packet_disconnect:
		{
			log("Disconnected!\n");
		} break;

		case e_packet_welcome:
		{
			s_welcome_from_server data = *(s_welcome_from_server*)cursor;
			game->my_id = data.id;
			game->current_level = data.current_level;
			game->rng.seed = data.seed;
			game->attempt_count_on_current_level = data.attempt_count_on_current_level;
			static_assert(sizeof(levels) == sizeof(data.levels));
			memcpy(levels, data.levels, sizeof(levels));
			int entity = make_player(data.id, true, game->config.color);
			game->e.name[entity] = game->main_menu.player_name;
			log("Got welcome, my id is: %u", data.id);
		} break;

		case e_packet_already_connected_player:
		{
			s_already_connected_player_from_server data = *(s_already_connected_player_from_server*)cursor;
			int entity = make_player(data.id, data.dead, data.color);
			game->e.name[entity] = data.name;
			game->e.color[entity] = data.color;
			log("Got already connected data of %u", data.id);
		} break;

		case e_packet_another_player_connected:
		{
			s_another_player_connected_from_server data = *(s_another_player_connected_from_server*)cursor;
			make_player(data.id, data.dead, v41f(1));
			log("Another player connected, id: %u", data.id);
		} break;

		case e_packet_player_update:
		{
			s_player_update_from_server data = *(s_player_update_from_server*)cursor;

			assert(data.id != game->my_id);
			int entity = find_player_by_id(data.id);
			if(entity != c_invalid_entity)
			{
				game->e.x[entity] = data.x;
				game->e.y[entity] = data.y;
			}

		} break;

		case e_packet_player_disconnected:
		{
			s_player_disconnected_from_server data = *(s_player_disconnected_from_server*)cursor;
			assert(data.id != game->my_id);
			int entity = find_player_by_id(data.id);
			if(entity != c_invalid_entity)
			{
				game->e.active[entity] = false;
			}
		} break;

		case e_packet_beat_level:
		{
			s_beat_level_from_server data = *(s_beat_level_from_server*)cursor;
			game->current_level = data.current_level + 1;
			game->rng.seed = data.seed;
			game->attempt_count_on_current_level = 0;
			reset_level();
			revive_every_player();
			log("Beat level %i", game->current_level);
			play_sound_if_supported(game->win_sound);
		} break;

		case e_packet_reset_level:
		{
			s_reset_level_from_server data = *(s_reset_level_from_server*)cursor;
			game->current_level = data.current_level;
			log("Reset level %i", game->current_level + 1);
			game->rng.seed = data.seed;
			game->attempt_count_on_current_level = data.attempt_count_on_current_level;
			reset_level();
			revive_every_player();
		} break;

		case e_packet_player_got_hit:
		{
			s_player_got_hit_from_server data = *(s_player_got_hit_from_server*)cursor;
			assert(data.id != game->my_id);
			int entity = find_player_by_id(data.id);
			if(entity != c_invalid_entity)
			{
				log("Player %i with id %u died", entity, data.id);
				game->e.dead[entity] = true;
			}
		} break;

		case e_packet_player_appearance:
		{
			s_player_appearance_from_server data = *(s_player_appearance_from_server*)cursor;
			assert(data.id != game->my_id);

			int entity = find_player_by_id(data.id);
			if(entity != c_invalid_entity)
			{
				game->e.name[entity] = data.name;
				game->e.color[entity] = data.color;
				log("Set %u's name to %s", data.id, game->e.name[entity].data);
			}
		} break;

		#ifdef m_debug
		case e_packet_cheat_previous_level:
		{
			s_cheat_previous_level_from_server data = *(s_cheat_previous_level_from_server*)cursor;
			game->current_level = data.current_level;
			game->rng.seed = data.seed;
			reset_level();
			revive_every_player();
		} break;
		#endif // m_debug

		case e_packet_all_levels_beat:
		{
			game->current_level = 0;
			reset_level();
			revive_every_player();
		} break;

		case e_packet_update_time_lived:
		{
			s_update_time_lived_from_server data = *(s_update_time_lived_from_server*)cursor;

			int entity = find_player_by_id(data.id);
			if(entity != c_invalid_entity)
			{
				game->e.time_lived[entity] = data.time_lived;
			}
		} break;

		case e_packet_update_levels:
		{
			s_update_levels_from_server data = *(s_update_levels_from_server*)cursor;
			memcpy(levels, data.levels, sizeof(levels));
			log("Got levels from server");
		} break;

		case e_packet_chat_msg:
		{
			s_chat_msg_from_server data = *(s_chat_msg_from_server*)cursor;
			assert(data.msg.len > 0 || data.msg.len <= data.msg.max_chars);

			int best_index = -1;
			for(int msg_i = 0; msg_i < c_max_peers; msg_i++)
			{
				if(game->chat_message_ids[msg_i] == data.id)
				{
					best_index = msg_i;
					break;
				}
				else if(!game->chat_message_ids[msg_i])
				{
					best_index = msg_i;
				}
			}

			// @Note(tkap, 27/06/2023): I think this could fail if we were at the maximum amount of clients, a chatting client disconnected, and another one
			// connected and chatted. Probably will never happen.
			assert(best_index != -1);

			game->chat_message_times[best_index] = 0;
			game->chat_message_ids[best_index] = data.id;
			game->chat_messages[best_index] = data.msg;

		} break;

		invalid_default_case;
	}
}
}


func void revive_every_player(void)
{
	for(int i = 0; i < c_max_entities; i++)
	{
		if(!game->e.active[i]) { continue; }
		if(!game->e.player_id[i]) { continue; }
		game->e.dead[i] = false;
		log("Revived player at index %i with id %u. I'm %u", i, game->e.player_id[i], game->my_id);
	}
}

func void collision_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!game->e.active[ii]) { continue; }
		if(!game->e.flags[ii][e_entity_flag_collide]) { continue; }

		for(int j = 0; j < c_max_entities; j++)
		{
			if(!game->e.active[j]) { continue; }
			if(game->e.dead[j]) { continue; }
			if(game->e.type[j] != e_entity_type_player) { continue; }
			if(game->e.player_id[j] != game->my_id) { continue; }

			if(
				rect_collides_circle(v2(game->e.x[j], game->e.y[j]), v2(game->e.sx[j], game->e.sy[j]), v2(game->e.x[ii], game->e.y[ii]), game->e.sx[ii] * 0.48f)
			)
			{
				game->e.dead[j] = true;
				s_player_got_hit_from_client data = zero;
				send_packet(e_packet_player_got_hit, data, ENET_PACKET_FLAG_RELIABLE);
			}
		}
	}
}

func s_font load_font(const char* path, float font_size, s_lin_arena* arena)
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

func s_v2 get_text_size_with_count(const char* text, e_font font_id, int count)
{
	assert(count >= 0);
	if(count <= 0) { return zero; }
	s_font* font = &game->font_arr[font_id];

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

func s_v2 get_text_size(const char* text, e_font font_id)
{
	return get_text_size_with_count(text, font_id, (int)strlen(text));
}


#ifdef m_debug
#ifdef _WIN32
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
			if(game->program)
			{
				glUseProgram(0);
				glDeleteProgram(game->program);
			}
			game->program = load_shader("shaders/vertex.vertex", "shaders/fragment.fragment");
			last_write_time = find_data.ftLastWriteTime;
		}
	}

	FindClose(handle);

}
#else
#include <sys/stat.h>
global time_t last_write_time;
func void hot_reload_shaders(void)
{
	struct stat s;
	stat("shaders/fragment.fragment", &s);
	if (s.st_mtime > last_write_time) {
		u32 new_program = load_shader("shaders/vertex.vertex", "shaders/fragment.fragment");
		if(new_program)
		{
			if(game->program)
			{
				glUseProgram(0);
				glDeleteProgram(game->program);
			}
			game->program = load_shader("shaders/vertex.vertex", "shaders/fragment.fragment");
			last_write_time = s.st_mtime;
		}
	}
}
#endif // _WIN32
#endif // m_debug

func u32 load_shader(const char* vertex_path, const char* fragment_path)
{
	u32 vertex = glCreateShader(GL_VERTEX_SHADER);
	u32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
	const char* header = "#version 430 core\n";
	char* vertex_src = read_file(vertex_path, frame_arena);
	if(!vertex_src || !vertex_src[0]) { return 0; }
	char* fragment_src = read_file(fragment_path, frame_arena);
	if(!fragment_src || !fragment_src[0]) { return 0; }
	const char* vertex_src_arr[] = {header, read_file("src/shader_shared.h", frame_arena), vertex_src};
	const char* fragment_src_arr[] = {header, read_file("src/shader_shared.h", frame_arena), fragment_src};
	glShaderSource(vertex, array_count(vertex_src_arr), (const GLchar * const *)vertex_src_arr, null);
	glShaderSource(fragment, array_count(fragment_src_arr), (const GLchar * const *)fragment_src_arr, null);
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
	game->e.prev_x[entity] = game->e.x[entity];
	game->e.prev_y[entity] = game->e.y[entity];
}

func void handle_instant_resize_(int entity)
{
	assert(entity != c_invalid_entity);
	game->e.prev_sx[entity] = game->e.sx[entity];
	game->e.prev_sy[entity] = game->e.sy[entity];
}

func s_config read_config_or_make_default(s_lin_arena* arena, s_rng* in_rng)
{
	s_config config = {};

	char* data = read_file("config.txt", arena);
	if(!data)
	{
		return make_default_config(in_rng);
	}

	struct s_query_data
	{
		const char* query;
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
					s_small_str* name = (s_small_str*)query.target;
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
	s_config config = {};
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

func s_small_str make_name(const char* str)
{
	s_small_str result = zero;
	int len = (int)strlen(str);
	assert(len <= result.max_chars);
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

func void send_simple_packet(e_packet packet_id, int flag)
{
	assert(flag == 0 || flag == ENET_PACKET_FLAG_RELIABLE);

	s_packet packet = zero;
	packet.size = sizeof(packet_id);
	packet.data = (u8*)la_get(&g_network->write_arena, packet.size);
	packet.flag = flag;
	*(e_packet*)packet.data = packet_id;
	g_network->out_packets.add(packet);
}

func void send_packet_(e_packet packet_id, void* data, size_t size, int flag)
{
	assert(flag == 0 || flag == ENET_PACKET_FLAG_RELIABLE);

	s_packet packet = zero;
	packet.size = (int)(size + sizeof(packet_id));
	packet.data = (u8*)la_get(&g_network->write_arena, packet.size);
	packet.flag = flag;
	u8* cursor = packet.data;
	buffer_write(&cursor, &packet_id, sizeof(packet_id));
	buffer_write(&cursor, data, size);
	g_network->out_packets.add(packet);
}

func void connect_to_server(s_config config)
{
	g_network->ip = config.ip;
	g_network->port = config.port;
	g_network->connect_to_server = true;
}

template <typename T>
func e_string_input_result handle_string_input(T* str)
{
	e_string_input_result result = e_string_input_result_none;
	while(true)
	{
		s_char_event event = get_char_event();
		int c = event.c;
		if(!c) { break; }

		if(event.is_symbol)
		{
			if(c == c_key_backspace)
			{
				if(str->len > 0)
				{
					str->data[--str->len] = 0;
				}
			}
			else if(c == c_key_enter)
			{
				result = e_string_input_result_submit;
				break;
			}
		}
		else
		{
			if(c >= 32 && c <= 126)
			{
				if(str->len < str->max_chars)
				{
					str->data[str->len++] = (char)c;
					str->data[str->len] = '\0';
				}
			}
		}
	}
	return result;
}

func void save_game_state(s_game* in_game)
{
	if(!state_file)
	{
		state_file = fopen("stored_state", "ab");
		assert(state_file);
	}

	la_push(&g_platform_data.frame_arena);
	u8* data = (u8*)la_get(&g_platform_data.frame_arena, sizeof(*in_game));

	u8* write_cursor = data;
	u8* read_cursor = (u8*)in_game;

	u8 last = *read_cursor;
	int start = 0;
	for(int i = 0; i < sizeof(*in_game); i++)
	{
		u8 current = *read_cursor;
		if(current != last || i == sizeof(*in_game) - 1)
		{
			int how_many = i - start;
			assert(how_many > 0);
			*(int*)write_cursor = how_many;
			write_cursor += sizeof(int);
			*write_cursor = last;
			write_cursor += 1;
			start = i;
		}
		last = current;
		read_cursor += 1;
	}

	int compressed_size = (int)(write_cursor - data);

	{
		assert(state_file);
		fwrite(&compressed_size, sizeof(compressed_size), 1, state_file);
		fwrite(data, compressed_size, 1, state_file);
	}

	// read_cursor = data;
	// write_cursor = (u8*)in_game;

	// for(int i = 0; i < compressed_size / 5; i++)
	// {
	// 	int how_many = *(int*)read_cursor;
	// 	read_cursor += sizeof(int);
	// 	u8 value = *read_cursor;
	// 	read_cursor += 1;
	// 	memset(write_cursor, value, how_many);
	// 	write_cursor += how_many;
	// }

	// printf("original game state size is: %i\n", (int)sizeof(*in_game));
	// printf("compressed game state size is: %i\n", compressed_size);
	// float ratio = compressed_size / (float)((int)sizeof(*in_game));
	// printf("compression ratio is: %f%%\n", (1.0f-ratio) * 100);


	la_pop(&g_platform_data.frame_arena);
}


func u8* get_game_state_for_frame(FILE* file, int target_frame)
{
	int current_frame = 0;
	u8* data = (u8*)la_get(&g_platform_data.frame_arena, sizeof(s_game));
	int frame_size = 0;
	fread(&frame_size, sizeof(frame_size), 1, file);

	while(current_frame != target_frame)
	{
		current_frame += 1;
		fseek(file, frame_size, SEEK_CUR);
		fread(&frame_size, sizeof(frame_size), 1, file);
	}

	fseek(file, -((int)sizeof(frame_size)), SEEK_CUR);
	fread(data, sizeof(frame_size) + sizeof(s_game), 1, file);

	return data;
}

func void load_game(s_game* in_game)
{
	if(!state_file)
	{
		state_file = fopen("stored_state", "rb");
		assert(state_file);
	}
	la_push(&g_platform_data.frame_arena);

	u8* data = get_game_state_for_frame(state_file, frame);

	u8* read_cursor = data;
	int compressed_size = *(int*)read_cursor;
	read_cursor += sizeof(compressed_size);
	u8* write_cursor = (u8*)in_game;

	for(int i = 0; i < compressed_size / 5; i++)
	{
		int how_many = *(int*)read_cursor;
		read_cursor += sizeof(int);
		u8 value = *read_cursor;
		read_cursor += 1;
		memset(write_cursor, value, how_many);
		write_cursor += how_many;
	}


	la_pop(&g_platform_data.frame_arena);
}