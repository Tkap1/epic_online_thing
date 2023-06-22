

#define zero {0}
#define func static
#define global static
#define null NULL
#define invalid_default_case default: { assert(false); }

#define error(b) do { if(!(b)) { printf("ERROR\n"); exit(1); }} while(0)
#define assert(b) do { if(!(b)) { printf("FAILED ASSERT at line %s (%i)\n", __FILE__, __LINE__); exit(1); }} while(0)
#define check(cond) do { if(!(cond)) { error(false); }} while(0)
#define unreferenced(thing) (void)thing;

#define c_kb (1024)
#define c_mb (1024 * 1024)

#define array_count(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define true 1
#define false 0

#define make_list(name, type, max_elements) typedef struct name { \
	int count; \
	type elements[max_elements]; \
} name; \
func void name##_add(name* list, type element) \
{ \
	assert(list->count < max_elements); \
	list->elements[list->count] = element; \
	list->count += 1; \
}

func void* buffer_read(u8** cursor, size_t size)
{
	void* result = *cursor;
	*cursor += size;
	return result;
}

func void buffer_write(u8** cursor, void* data, size_t size)
{
	memcpy(*cursor, data, size);
	*cursor += size;
}