
global u64 g_cycle_frequency;
global u64 g_start_cycles;

#ifdef _WIN32
func f64 get_seconds(void)
{
	u64 now;
	QueryPerformanceCounter((LARGE_INTEGER*)&now);
	return (now - g_start_cycles) / (f64)g_cycle_frequency;
}

func void init_performance(void)
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&g_cycle_frequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&g_start_cycles);
}
#else
#include <time.h>

func u64 ticks(struct timespec ts)
{
	return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

func f64 get_seconds(void)
{
	struct timespec ts;
	timespec_get(&ts, TIME_UTC);
	u64 now = ticks(ts);
	f64 seconds = (f64)(now - g_start_cycles) / (f64)1000000000.0;
	return seconds;
}

func void init_performance(void)
{
	struct timespec ts;
	g_cycle_frequency = 1000000000L;
	timespec_get(&ts, TIME_UTC);
	g_start_cycles = ticks(ts);
}
#endif
