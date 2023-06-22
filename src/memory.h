
#define c_max_arena_push 16

typedef struct s_lin_arena
{
	int push_count;
	size_t push[c_max_arena_push];
	size_t used;
	size_t capacity;
	void* memory;
} s_lin_arena;

