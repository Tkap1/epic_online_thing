

global constexpr s_v2 c_base_res = {1366, 768};
global constexpr s_v2 c_spawn_pos = {c_base_res.x * 0.5f, c_base_res.y * 0.5f};

global constexpr int c_num_channels = 2;
global constexpr int c_sample_rate = 44100;
global constexpr int c_max_concurrent_sounds = 8;

#define c_updates_per_second (100)
// #define c_updates_per_second (5)
#define c_update_delay (1.0 / c_updates_per_second)

#define c_origin_topleft {1.0f, -1.0f}
#define c_origin_bottomleft {1.0f, 1.0f}
#define c_origin_center {0, 0}

#define c_num_threads (1)
#define c_max_entities (4096)
#define c_entities_per_thread (c_max_entities / c_num_threads)

#define delta (1.0f / c_updates_per_second)
#define c_invalid_entity (-1)
#define c_gravity (1700)
#define c_jump_strength (-700)
#define c_max_levels (100)
#define c_level_duration (15)
#define c_fast_fall_speed (600)

#define max_player_name_length 32
#define c_str_builder_size 1024

#define c_projectile_spawn_offset (100)
#define c_projectile_out_of_bounds_offset (200)

global constexpr int c_max_spawns_per_level = 16;