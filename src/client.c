
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <gl/GL.h>
#include "external/glcorearb.h"
#include "external/wglext.h"

#include <winsock2.h>
#include <stdio.h>
#include "external\enet\enet.h"
#include "types.h"
#include "utils.h"
#include "math.h"
#include "config.h"
#include "shared.h"
#include "time.h"
#include "window.h"
#include "memory.h"
#include "file.h"
#include "client.h"
#include "shader_shared.h"

make_list(s_transform_list, s_transform, 1024);
s_transform_list transforms;

global s_entities e;
global u32 my_id = 0;
global ENetPeer* server;
global s_lin_arena frame_arena;

#include "draw.c"
#include "memory.c"
#include "file.c"
#include "window.c"
#include "shared.c"


int main(int argc, char** argv)
{
	argc -= 1;
	argv += 1;

	assert((c_max_entities % c_num_threads) == 0);

	init_performance();

	if(enet_initialize() != 0)
	{
		error(false);
	}

	frame_arena = make_lin_arena(1 * c_mb);

	ENetHost* client = enet_host_create(
		null /* create a client host */,
		1, /* only allow 1 outgoing connection */
		2, /* allow up 2 channels to be used, 0 and 1 */
		0, /* assume any amount of incoming bandwidth */
		0 /* assume any amount of outgoing bandwidth */
	);

	if(client == null)
	{
		error(false);
	}

	ENetAddress address = zero;
	enet_address_set_host(&address, "at-taxation.at.ply.gg");
	// enet_address_set_host(&address, "127.0.0.1");
	address.port = 62555;

	server = enet_host_connect(client, &address, 2, 0);
	if(server == null)
	{
		error(false);
	}

	create_window();

	if(wglSwapIntervalEXT)
	{
		wglSwapIntervalEXT(1);
	}

	u32 vao;
	u32 ssbo;
	u32 program;
	{
		u32 vertex = glCreateShader(GL_VERTEX_SHADER);
		u32 fragment = glCreateShader(GL_FRAGMENT_SHADER);
		char* header = "#version 430 core\n";
		char* vertex_src = read_file_quick("shaders/vertex.vertex", &frame_arena);
		char* fragment_src = read_file_quick("shaders/fragment.fragment", &frame_arena);
		char* vertex_src_arr[] = {header, read_file_quick("src/shader_shared.h", &frame_arena), vertex_src};
		char* fragment_src_arr[] = {header, read_file_quick("src/shader_shared.h", &frame_arena), fragment_src};
		glShaderSource(vertex, array_count(vertex_src_arr), vertex_src_arr, null);
		glShaderSource(fragment, array_count(fragment_src_arr), fragment_src_arr, null);
		glCompileShader(vertex);
		char buffer[1024] = zero;
		check_for_shader_errors(vertex, buffer);
		glCompileShader(fragment);
		check_for_shader_errors(fragment, buffer);
		program = glCreateProgram();
		glAttachShader(program, vertex);
		glAttachShader(program, fragment);
		glLinkProgram(program);
		glUseProgram(program);
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(transforms.elements), null, GL_DYNAMIC_DRAW);

	b8 running = true;
	f64 update_timer = 0;
	while(running)
	{

		f64 start_of_frame_seconds = get_seconds();

		ENetEvent event = zero;
		while(enet_host_service(client, &event, 0) > 0)
		{
			switch(event.type)
			{
				case ENET_EVENT_TYPE_NONE:
				{
				} break;

				case ENET_EVENT_TYPE_CONNECT:
				{
					printf("Client: connected!\n");
				} break;

				case ENET_EVENT_TYPE_DISCONNECT:
				{
					printf("Client: disconnected!\n");
				} break;

				case ENET_EVENT_TYPE_RECEIVE:
				{
					parse_packet(event);
					enet_packet_destroy(event.packet);
				} break;

				invalid_default_case;
			}
		}

		MSG msg = zero;
		while(PeekMessage(&msg, null, 0, 0, PM_REMOVE) > 0)
		{
			if(msg.message == WM_QUIT)
			{
				running = false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		update_timer += delta;
		while(update_timer >= c_update_delay)
		{
			update_timer -= c_update_delay;
			update();
		}

		render(program);

		for(int k_i = 0; k_i < c_max_keys; k_i++)
		{
			g_input.keys[k_i].count = 0;
		}
		frame_arena.used = 0;

		SwapBuffers(g_window.dc);


		time_passed = (float)(get_seconds() - start_of_frame_seconds);
		total_time += time_passed;
	}

	return 0;
}


func void update()
{
	for(int i = 0; i < c_num_threads; i++)
	{
		input_system(i * c_entities_per_thread, c_entities_per_thread);
	}
	for(int i = 0; i < c_num_threads; i++)
	{
		move_system(i * c_entities_per_thread, c_entities_per_thread);
	}

	int my_player = find_player_by_id(my_id);
	if(my_player != c_invalid_entity)
	{
		e_packet packet_id = e_packet_player_update;
		u8* data = la_get(&frame_arena, 1024);
		u8* cursor = data;
		buffer_write(&cursor, &packet_id, sizeof(packet_id));
		buffer_write(&cursor, &e.x[my_player], sizeof(e.x[my_player]));
		buffer_write(&cursor, &e.y[my_player], sizeof(e.y[my_player]));
		ENetPacket* packet = enet_packet_create(data, cursor - data, 0);
		enet_peer_send(server, 0, packet);
	}

}

func void render(u32 program)
{

	for(int i = 0; i < c_num_threads; i++)
	{
		draw_system(i * c_entities_per_thread, c_entities_per_thread);
	}

	{
		// glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClearDepth(0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, g_window.width, g_window.height);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_GREATER);

		int location = glGetUniformLocation(program, "window_size");
		glUniform2fv(location, 1, &g_window.size.x);

		if(transforms.count > 0)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*transforms.elements) * transforms.count, transforms.elements);
			glDrawArraysInstanced(GL_TRIANGLES, 0, 6, transforms.count);
			transforms.count = 0;
		}

		// for(int font_i = 0; font_i < e_font_count; font_i++)
		// {
		// 	if(text_arr[font_i].count > 0)
		// 	{
		// 		s_font* font = &g_font_arr[font_i];
		// 		glBindTexture(GL_TEXTURE_2D, font->texture.id);
		// 		glEnable(GL_BLEND);
		// 		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		// 		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(*text_arr[font_i].elements) * text_arr[font_i].count, text_arr[font_i].elements);
		// 		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, text_arr[font_i].count);
		// 		text_arr[font_i].count = 0;
		// 	}
		// }
	}
}

func b8 check_for_shader_errors(u32 id, char* out_error)
{
	int compile_success;
	char info_log[1024];
	glGetShaderiv(id, GL_COMPILE_STATUS, &compile_success);

	if(!compile_success)
	{
		glGetShaderInfoLog(id, 1024, null, info_log);
		printf("Failed to compile shader:\n%s", info_log);

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
	}
}

func void draw_system(int start, int count)
{
	for(int i = 0; i < count; i++)
	{
		int ii = start + i;
		if(!e.active[ii]) { continue; }
		if(!e.flags[ii][e_entity_flag_draw]) { continue; }

		draw_rect(v2(e.x[ii], e.y[ii]), 0, v2(e.sx[ii], e.sy[ii]), v41f(1), (s_transform)zero);

	}
}

func void parse_packet(ENetEvent event)
{
	u8* cursor = event.packet->data;
	e_packet packet_id = *(e_packet*)buffer_read(&cursor, sizeof(packet_id));

	switch(packet_id)
	{
		case e_packet_welcome:
		{
			u32 player_id = *(u32*)buffer_read(&cursor, sizeof(player_id));
			my_id = player_id;
			make_player(player_id);
		} break;

		case e_packet_another_player_connected:
		{
			u32 player_id = *(u32*)buffer_read(&cursor, sizeof(player_id));
			int entity = make_player(player_id);
			e.player_id[entity] = player_id;
		} break;

		case e_packet_player_update:
		{
			u32 id = *(u32*)buffer_read(&cursor, sizeof(u32));
			float x = *(float*)buffer_read(&cursor, sizeof(float));
			float y = *(float*)buffer_read(&cursor, sizeof(float));

			assert(id != my_id);
			int entity = find_player_by_id(id);
			if(entity != c_invalid_entity)
			{
				e.x[entity] = x;
				e.y[entity] = y;
			}

		} break;

		invalid_default_case;
	}
}

func int make_player(u32 player_id)
{
	int entity = make_entity();
	e.x[entity] = 100;
	e.y[entity] = 100;
	e.sx[entity] = 64;
	e.sy[entity] = 64;
	e.player_id[entity] = player_id;
	e.speed[entity] = 1000;
	e.flags[entity][e_entity_flag_move] = true;
	e.flags[entity][e_entity_flag_draw] = true;
	if(player_id == my_id)
	{
		e.flags[entity][e_entity_flag_input] = true;
	}
	return entity;
}