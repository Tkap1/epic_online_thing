
#pragma pack(push, 1)
struct s_riff_chunk
{
	u32 chunk_id;
	u32 chunk_size;
	u32 format;
};

struct s_fmt_chunk
{
	u32 sub_chunk1_id;
	u32 sub_chunk1_size;
	u16 audio_format;
	u16 num_channels;
	u32 sample_rate;
	u32 byte_rate;
	u16 block_align;
	u16 bits_per_sample;
};

struct s_data_chunk
{
	u32 sub_chunk2_id;
	u32 sub_chunk2_size;
};
#pragma pack(pop)

struct s_sound
{
	int sample_count;
	s16* samples;
};

struct s_voice : IXAudio2VoiceCallback
{
	IXAudio2SourceVoice* voice;

	volatile int playing;
	// s_sound sound;

	void OnStreamEnd()
	{
		// assert(sound.sample_count > 0);
		// assert(sound.samples);
		// free(sound.samples);
		InterlockedExchange((LONG*)&playing, false);
		voice->Stop();
	}

	void OnBufferStart(void * pBufferContext)
	{
		unreferenced(pBufferContext);
		InterlockedExchange((LONG*)&playing, true);
	}

	void OnVoiceProcessingPassEnd() { }
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) { unreferenced(SamplesRequired); }
	void OnBufferEnd(void * pBufferContext) { unreferenced(pBufferContext); }
	void OnLoopEnd(void * pBufferContext) { unreferenced(pBufferContext); }
	void OnVoiceError(void * pBufferContext, HRESULT Error) { unreferenced(pBufferContext); unreferenced(Error);}
};

func b8 init_audio();
func b8 thread_safe_set_bool_to_true(volatile int* var);