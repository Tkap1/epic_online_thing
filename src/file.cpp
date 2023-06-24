
// func s_read_file_result read_file(char* path, s_lin_arena* arena)
// {
// 	s_read_file_result result = zero;
// 	result.file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
// 	if(result.file == INVALID_HANDLE_VALUE) { return result; }

// 	result.file_size = GetFileSize(result.file, null);
// 	result.data = (char*)la_get(arena, result.file_size + 1);

// 	BOOL read_result = ReadFile(result.file, result.data, result.file_size, &result.bytes_read, null);
// 	if(read_result == 0)
// 	{
// 		result.data = null;
// 	}
// 	else
// 	{
// 		result.success = true;
// 		result.data[result.bytes_read] = 0;
// 	}
// 	return result;
// }


// func char* read_file_quick(char* path, s_lin_arena* arena)
// {
// 	s_read_file_result result = read_file(path, arena);
// 	if(result.file != INVALID_HANDLE_VALUE) { CloseHandle(result.file); }
// 	return result.data;
// }


func char* read_file(char* path, s_lin_arena* arena)
{
	FILE* file = fopen(path, "rb");
	if(!file) { return null; }

	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* data = (char*)la_get(arena, file_size + 1);
	fread(data, file_size, 1, file);
	data[file_size] = 0;
	fclose(file);
	return data;
}

func b8 write_file(char* path, void* data, size_t size)
{
	assert(size > 0);
	FILE* file = fopen(path, "wb");
	if(!file) { return false; }

	fwrite(data, size, 1, file);
	fclose(file);
	return true;
}