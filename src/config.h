

global s_v2 c_base_res = {1366, 768};
#define c_updates_per_second 60
#define c_update_delay (1.0 / c_updates_per_second)

#define c_num_threads 1
#define c_max_entities 1024
#define c_entities_per_thread (c_max_entities / c_num_threads)

#define delta (1.0f / c_updates_per_second)
#define c_invalid_entity (-1)