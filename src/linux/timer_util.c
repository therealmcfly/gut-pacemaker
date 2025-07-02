#include "timer_util.h"
#include "global.h" // for g_shared_data
#include <stdio.h>

void timer_start(Timer *t)
{
	if (t == NULL)
	{
		perror("Error: NULL pointer passed to timer_start\n");
		return;
	}
	if (t->is_running)
	{
		printf("\nError: Timer is already running.\n");
	}
	else
	{
		clock_gettime(CLOCK_MONOTONIC, &t->start_time);
		t->is_running = 1; // Set the timer as running
	}
}

void timer_stop(Timer *t)
{
	if (t == NULL)
	{
		perror("Error: NULL pointer passed to timer_stop\n");
		return;
	}
	if (!t->is_running)
	{
		printf("\nError: Timer is not running. Start the timer before attempting to stop it.\n");
		return;
	}
	else
	{
		clock_gettime(CLOCK_MONOTONIC, &t->end_time);
		t->is_running = 0; // Set the timer as not running
	}
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

void initialize_et_timer(Timer *t)
{
	if (t == NULL)
	{
		perror("Error: NULL pointer passed to initialize_timer\n");
		return;
	}
	t->is_running = 0; // Initialize the timer as not running
	t->wc_flag = 0;		 // Initialize the worst-case execution time flag
	t->et = 0;				 // Initialize the execution time
	t->wcet = 0;			 // Initialize the worst-case execution time
}

void get_n_set_execution_time(Timer *t)
{
	if (t == NULL)
	{
		perror("Error: NULL pointer passed to get_n_set_execution_time\n");
		return;
	}
	if (!t->is_running)
	{
		printf("\nError: Timer is not running. Start the timer before attempting to stop it.\n");
		return;
	}
	t->wc_flag = 0; // Flag to indicate if the worst-case execution time is updated
	timer_stop(t);
	// Stop the timer to get execution time
	t->et = timer_elapsed_ms(t);
	// Store the execution time in milliseconds
	if (t->et > t->wcet)
	{
		// Update the worst-case execution time if current execution time is greater
		t->wcet = t->et;
		t->wc_flag = 1; // Set the flag to indicate that the worst-case execution time is updated
	}
}