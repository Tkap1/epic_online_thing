

#define c_base_res (s_v2){1366, 768}
#define c_spawn_pos (s_v2){c_base_res.x * 0.5f, c_base_res.y * 0.5f}
#define c_updates_per_second (100)
#define c_update_delay (1.0 / c_updates_per_second)

#define c_origin_topleft (s_v2){1.0f, -1.0f}
#define c_origin_bottomleft (s_v2){1.0f, 1.0f}
#define c_origin_center (s_v2){0, 0}

#define c_num_threads (1)
#define c_max_entities (1024)
#define c_entities_per_thread (c_max_entities / c_num_threads)

#define delta (1.0f / c_updates_per_second)
#define c_invalid_entity (-1)
#define c_gravity (1700)
#define c_jump_strength (-700)
#define c_max_levels (100)
#define c_level_duration (15)
#define c_fast_fall_speed (600)

#define max_player_name_length 16