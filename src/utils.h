

#define zero {0}
#define func static
#define global static
#define null NULL
#define invalid_default_case default: { assert(false); }

#define error(b) do { if(!(b)) { printf("ERROR\n"); exit(1); }} while(0)
#define assert(cond) do { if(!(cond)) { on_failed_assert(#cond, __FILE__, __LINE__); } } while(0)
#define check(cond) do { if(!(cond)) { error(false); }} while(0)
#define unreferenced(thing) (void)thing;

#define c_kb (1024)
#define c_mb (1024 * 1024)

#define array_count(arr) (sizeof((arr)) / sizeof((arr)[0]))

#define true 1
#define false 0

#ifdef m_server
#define log(...) printf("Server: "  __VA_ARGS__); printf("\n")
#else // m_server
#define log(...) printf("Client: " __VA_ARGS__); printf("\n")
#endif

func void on_failed_assert(char* cond, char* file, int line);

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

func char* format_text(char* text, ...)
{
	#define max_format_text_buffers 16
	#define max_text_buffer_length 256

	static char buffers[max_format_text_buffers][max_text_buffer_length] = zero;
	static int index = 0;

	char* current_buffer = buffers[index];
	memset(current_buffer, 0, max_text_buffer_length);

	va_list args;
	va_start(args, text);
	#ifdef m_debug
	int written = vsnprintf(current_buffer, max_text_buffer_length, text, args);
	assert(written > 0 && written < max_text_buffer_length);
	#else
	vsnprintf(current_buffer, max_text_buffer_length, text, args);
	#endif
	va_end(args);

	index += 1;
	if(index >= max_format_text_buffers) { index = 0; }

	return current_buffer;
}

func void on_failed_assert(char* cond, char* file, int line)
{
	char* text = format_text("FAILED ASSERT IN %s (%i)\n%s\n", file, line, cond);
	printf("%s\n", text);
	int result = MessageBox(null, text, "Assertion failed", MB_RETRYCANCEL | MB_TOPMOST);
	if(result != IDRETRY)
	{
		if(IsDebuggerPresent())
		{
			__debugbreak();
		}
		else
		{
			exit(1);
		}
	}
}


template <typename T, int N>
struct s_sarray
{
	static_assert(N > 0);
	int count = 0;
	T elements[N];

	constexpr T& operator[](int index)
	{
		assert(index >= 0);
		assert(index < count);
		return elements[index];
	}

	constexpr T get(int index)
	{
		return (*this)[index];
	}

	T pop()
	{
		assert(count > 0);
		return elements[--count];
	}

	constexpr void remove_and_swap(int index)
	{
		assert(index >= 0);
		assert(index < count);
		count -= 1;
		elements[index] = elements[count];
	}

	constexpr T remove_and_shift(int index)
	{
		assert(index >= 0);
		assert(index < count);
		T result = elements[index];
		count -= 1;

		int to_move = count - index;
		if(to_move > 0)
		{
			memcpy(elements + index, elements + index + 1, to_move * sizeof(T));
		}
		return result;
	}

	constexpr T* get_ptr(int index)
	{
		return &(*this)[index];
	}

	constexpr void swap(int index0, int index1)
	{
		assert(index0 >= 0);
		assert(index1 >= 0);
		assert(index0 < count);
		assert(index1 < count);
		assert(index0 != index1);
		T temp = elements[index0];
		elements[index0] = elements[index1];
		elements[index1] = temp;
	}

	constexpr T get_last()
	{
		assert(count > 0);
		return elements[count - 1];
	}

	constexpr T* get_last_ptr()
	{
		assert(count > 0);
		return &elements[count - 1];
	}

	constexpr int add(T element)
	{
		assert(count < N);
		elements[count] = element;
		count += 1;
		return count - 1;
	}

	constexpr b8 add_checked(T element)
	{
		if(count < N)
		{
			add(element);
			return true;
		}
		return false;
	}

	constexpr b8 contains(T what)
	{
		for(int element_i = 0; element_i < count; element_i++)
		{
			if(what == elements[element_i])
			{
				return true;
			}
		}
		return false;
	}

	constexpr void insert(int index, T element)
	{
		assert(index >= 0);
		assert(index < N);
		assert(index <= count);

		int to_move = count - index;
		count += 1;
		if(to_move > 0)
		{
			memmove(&elements[index + 1], &elements[index], to_move * sizeof(T));
		}
		elements[index] = element;
	}

	constexpr int max_elements()
	{
		return N;
	}

	constexpr b8 is_last(int index)
	{
		assert(index >= 0);
		assert(index < count);
		return index == count - 1;
	}

	constexpr b8 is_full()
	{
		return count >= N;
	}

	b8 is_empty()
	{
		return count <= 0;
	}

	void bubble_sort()
	{
		// @Note(tkap, 25/06/2023): Let's not get crazy with bubble sort, bro
		assert(count < 256);

		for(int i = 0; i < count; i++)
		{
			b8 swap = false;
			for(int j = 0; j < count - 1; j++)
			{
				T* a = &elements[j];
				T* b = &elements[j + 1];

				if(*a > *b)
				{
					T temp = *a;
					*a = *b;
					*b = temp;
					swap = true;
				}
			}
			if(!swap) { break; }
		}
	}

};