
func void add_job(t_thread_func f, s_args args)
{
	assert(thread_queue.funcs_count < 16);
	thread_queue.funcs[thread_queue.funcs_count] = f;
	thread_queue.args[thread_queue.funcs_count++] = args;
}

func void start_jobs()
{
	assert(thread_queue.funcs_count > 0);
	InterlockedExchange((LONG*)&thread_queue.jobs_done, thread_queue.funcs_count);
	InterlockedExchange((LONG*)&thread_queue.jobs_assigned, thread_queue.funcs_count);
}

func void wait_for_jobs()
{
	while(thread_queue.jobs_done > 0) {}
	thread_queue.funcs_count = 0;
}

func DWORD thread_spin(void* param)
{
	unreferenced(param);
	DWORD id = GetCurrentThreadId();
	while(true)
	{
		int jobs_assigned_belief = thread_queue.jobs_assigned;
		if(jobs_assigned_belief > 0)
		{
			int index = InterlockedDecrement((LONG*)&thread_queue.jobs_assigned);
			if(index >= 0)
			{
				s_args args = thread_queue.args[index];
				thread_queue.funcs[index](args.a, args.b);
				InterlockedDecrement((LONG*)&thread_queue.jobs_done);
			}
		}
		else
		{
			Sleep(0);
		}
	}
	return 0;
}