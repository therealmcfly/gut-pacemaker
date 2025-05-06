#include "timer_util.h"

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