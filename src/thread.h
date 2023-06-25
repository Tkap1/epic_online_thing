
typedef void (*t_thread_func)(int, int);

struct s_args
{
	int a;
	int b;
};

struct s_thread_queue
{
	volatile int jobs_assigned;
	volatile int jobs_done;
	int funcs_count;
	s_args args[16];
	t_thread_func funcs[16];
};

global s_thread_queue thread_queue;