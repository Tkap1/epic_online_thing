
func s_sound load_wav(const char* path, s_lin_arena* arena)
{

	s_sound result = zero;
	u8* data = (u8*)read_file(path, arena);
	if(!data) { return zero; }

	s_riff_chunk riff = *(s_riff_chunk*)data;
	data += sizeof(riff);
	s_fmt_chunk fmt = *(s_fmt_chunk*)data;
	assert(fmt.num_channels == c_num_channels);
	assert(fmt.sample_rate == c_sample_rate);
	data += sizeof(fmt);
	s_data_chunk data_chunk = *(s_data_chunk*)data;
	assert(memcmp(&data_chunk.sub_chunk2_id, "data", 4) == 0);
	data += 8;

	result.sample_count = data_chunk.sub_chunk2_size / c_num_channels / sizeof(s16);
	result.samples = (s16*)malloc(c_num_channels * sizeof(s16) * result.sample_count);
	memcpy(result.samples, data, result.sample_count * c_num_channels * sizeof(s16));

	return result;
}

func b8 play_sound_if_supported(s_sound sound)
{
	if(!g_platform_funcs.play_sound) { return false; }
	return g_platform_funcs.play_sound(sound);
}
