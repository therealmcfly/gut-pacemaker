#include "timer_util.h"
#include "global.h" // for g_shared_data

void timer_start(Timer *t)
{
	clock_gettime(CLOCK_MONOTONIC, &t->start_time);
}

void timer_stop(Timer *t)
{
	clock_gettime(CLOCK_MONOTONIC, &t->end_time);
}

double timer_elapsed_ms(const Timer *t)
{
	return (t->end_time.tv_sec - t->start_time.tv_sec) * 1000.0 +
				 (t->end_time.tv_nsec - t->start_time.tv_nsec) / 1e6;
}

double timer_elapsed_us(const Timer *t)
{
	return (t->end_time.tv_sec - t->start_time.tv_sec) * 1e6 +
				 (t->end_time.tv_nsec - t->start_time.tv_nsec) / 1e3;
}

void initialize_timer_ptr(Timer *t, double *exec_time_ptr, double *wcet_ptr)
{
	if (t == NULL)
	{
		perror("Error: NULL pointer passed to initialize_timer\n");
		return;
	}
	g_shared_data.timer_ptr = t;										// pointer to timer_ptr for interval processing
	g_shared_data.exec_time_ptr = exec_time_ptr; // pointer to execution time in milliseconds
	g_shared_data.wcet_ptr = wcet_ptr;							// pointer to worst-case execution time
}