#ifndef TIMER_UTIL_H
#define TIMER_UTIL_H

#include <time.h> // for timespec

typedef struct
{
	int is_running; // Flag to indicate if the timer is running
	int wc_flag;
	double et;
	double wcet;
	struct timespec start_time;
	struct timespec end_time;
} Timer;

void timer_start(Timer *t);
void timer_stop(Timer *t);
double timer_elapsed_ms(const Timer *t);
double timer_elapsed_us(const Timer *t); // Optional: microsecond precision

void initialize_et_timer(Timer *t);
void get_n_set_execution_time(Timer *t);

#endif