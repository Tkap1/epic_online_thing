
global FILETIME last_dll_write_time;

func b8 need_to_reload_dll(char* path)
{
	WIN32_FIND_DATAA find_data = zero;
	HANDLE handle = FindFirstFileA(path, &find_data);
	if(handle == INVALID_HANDLE_VALUE) { assert(false); return false; }

	b8 result = CompareFileTime(&last_dll_write_time, &find_data.ftLastWriteTime) == -1;
	FindClose(handle);
	if(result)
	{
		last_dll_write_time = find_data.ftLastWriteTime;
	}
	return result;
}

func HMODULE load_dll(char* path)
{
	HMODULE result = LoadLibrary(path);
	assert(result);
	return result;
}

func void unload_dll(HMODULE dll)
{
	check(FreeLibrary(dll));
}