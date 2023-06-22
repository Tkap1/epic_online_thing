


func s_lin_arena make_lin_arena(size_t capacity)
{
	assert(capacity > 0);
	capacity = (capacity + 7) & ~7;
	s_lin_arena result = zero;
	result.capacity = capacity;
	result.memory = VirtualAlloc(null, capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return result;
}

func void* la_get(s_lin_arena* arena, size_t in_requested)
{
	assert(in_requested > 0);
	size_t requested = (in_requested + 7) & ~7;
	assert(arena->used + requested <= arena->capacity);
	void* result = (u8*)arena->memory + arena->used;
	arena->used += requested;
	return result;
}
