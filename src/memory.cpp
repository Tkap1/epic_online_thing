

#ifdef _WIN32
func s_lin_arena make_lin_arena(size_t capacity)
{
	assert(capacity > 0);
	capacity = (capacity + 7) & ~7;
	s_lin_arena result = zero;
	result.capacity = capacity;
	result.memory = VirtualAlloc(null, capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return result;
}
#else
func s_lin_arena make_lin_arena(size_t capacity)
{
	assert(capacity > 0);
	u64 pagesize = sysconf(_SC_PAGESIZE);
	capacity = (capacity + (pagesize-1)) & ~(pagesize-1);
	s_lin_arena result = zero;
	result.capacity = capacity;
	result.memory = aligned_alloc(pagesize, capacity);
	return result;
}
#endif

func void* la_get(s_lin_arena* arena, size_t in_requested)
{
	assert(in_requested > 0);
	size_t requested = (in_requested + 7) & ~7;
	assert(arena->used + requested <= arena->capacity);
	void* result = (u8*)arena->memory + arena->used;
	arena->used += requested;
	return result;
}

func void la_push(s_lin_arena* arena)
{
	assert(arena->push_count < c_max_arena_push);
	arena->push[arena->push_count++] = arena->used;
}

func void la_pop(s_lin_arena* arena)
{
	assert(arena->push_count > 0);
	arena->used = arena->push[--arena->push_count];
}
