

typedef struct s_read_file_result
{
	b8 success;
	int file_size;
	DWORD bytes_read;
	HANDLE file;
	u64 last_write_time;
	char* data;
} s_read_file_result;
