#include "global.h"
#include "pacemaker.h"
#include <stdio.h>

#define STR_WIDTH 25
double snapshot_buffer[ACTIVATION_DETECTION_BUFFER_SIZE]; // Buffer to hold the snapshot of the ring buffer

typedef enum
{
	PM_LEARNING = 0,	// Learning state
	PM_DETECTING = 1, // Detecting state
	PM_IGNORING = 2,	// Ignoring state
	PM_PACING = 3			// Pacing state
} PmState;

static void print_waiting(ChannelData *ch_data, int timer, int waiting, int detection)
{
	const char *str;
	if (!waiting)
	{
		if (detection)
		{
			str = "DETECTION IGNORED";
		}
		else
			str = "Ignoring...";
	}
	else
	{
		str = "Waiting...";
	}

	printf("\r[%.2fs][WCET%.2fms][ET%.2fms] %-*s ACT %d / LRI %d / GRI %d / PACE %d", (float)timer / 1000.0f, ch_data->et_timer_ptr->wcet, ch_data->et_timer_ptr->et, STR_WIDTH, str, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_flag);
	if (ch_data->et_timer_ptr->et > 10.0)
	{
		printf("\n");
		perror("Error: ET is greater than 10ms. This may cause problems. Please check your implementation.\n");
		return;
	}
	if (g_shared_data.buffer_skipped || ch_data->et_timer_ptr->wc_flag)
	{
		printf("\n");
		if (g_shared_data.buffer_skipped)
			g_shared_data.buffer_skipped = 0; // Reset the buffer skipped flag
		return;
	}

	if (!detection)
		fflush(stdout); // Print count and clear leftovers
}

static void print_detection(ChannelData *ch_data, int timer, int detection, int pacing)
{
	// // Lock the mutex to ensure thread safety
	// timer_stop(g_shared_data.timer_ptr);
	// // Stop the timer to get execution time
	// *g_shared_data.exec_time_ptr = timer_elapsed_ms(g_shared_data.timer_ptr);
	// // Store the execution time in milliseconds
	// if (*g_shared_data.exec_time_ptr > *g_shared_data.wcet_ptr)
	// {
	// 	*g_shared_data.wcet_ptr = *g_shared_data.exec_time_ptr; // Update the worst-case execution time if current execution time is greater
	// }

	char *str;
	if (detection)
	{
		if (pacing > 0)
		{
			str = "Activation after pacing!";
		}
		else
		{
			str = "Activation detected!";
		}
	}
	else
	{
		str = "PACING!";
	}
	printf("\r[%.2fs][WCET%.2fms][ET%.2fms] %-*s ACT %d / LRI %d / GRI %d / PACE %d", (float)timer / 1000.0f, ch_data->et_timer_ptr->wcet, ch_data->et_timer_ptr->et, STR_WIDTH, str, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_flag);
	if (ch_data->et_timer_ptr->et > 100.0)
	{
		printf("\n");
		perror("Error: ET is greater than 10ms. This may cause problems. Please check your implementation.\n");
		return;
	}
	if (g_shared_data.buffer_skipped)
	{
		printf("\n");
		g_shared_data.buffer_skipped = 0; // Reset the buffer skipped flag
		return;
	}
	fflush(stdout); // Print count and clear leftovers
}

static void print_pacing(ChannelData *ch_data, int timer)
{
	char *str = "PACING!";
	printf("\n[%.2fs][WCET%.2fms][ET%.2fms] %-*s ACT %d / LRI %d / GRI %d / PACE %d\n", (float)timer / 1000.0f, ch_data->et_timer_ptr->wcet, ch_data->et_timer_ptr->et, STR_WIDTH, str, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_flag);
	if (ch_data->et_timer_ptr->et > 100.0)
	{
		perror("Error: ET is greater than 10ms. This may cause problems. Please check your implementation.\n");
	}
	if (g_shared_data.buffer_skipped)
	{
		g_shared_data.buffer_skipped = 0; // Reset the buffer skipped flag
		return;
	}

	// printf("\n%sActivation detected at %.2f seconds.\n\t\t\t\tACT %d\n\t\tLRI %d.\n\t\tGRI %d\n\t\tPACE %d\n", RT_TITLE, (float)timer / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_flag);
}

static double get_lowest_scope(ChannelData *ch_data, double *buffer)
{
	double lowest_slope = (buffer[1] - buffer[0]) / ((double)g_samp_interval_ms / 1000); // initial slope set
	for (int i = 2; i < ch_data->ch_rb_ptr->size; i++)
	{
		// slope = (y2 - y1) / (x2 - x1) = (current sample - prev sample) / g_samp_interval_ms
		double slope = (buffer[i] - buffer[i - 1]) / ((double)g_samp_interval_ms / 1000);

		if (slope < lowest_slope)
		{
			lowest_slope = slope;
		}

		// Lock the mutex to safely update shared data
		// printf("Sample[%d]: %.10f, Slope[%d]: %.10f\n", i, signal_buffer[i], i, shared_data->slope[i]);
		// printf("Slope[%d]: %f\n", i, shared_data->slope[i]);
	}

	return lowest_slope; // Return success
}

static void reset_timers(ChannelData *ch_data, PacemakerData *p_data)
{
	ch_data->lri_ms = 0;										 // reset lri_ms
	ch_data->gri_ms = p_data->gri_thresh_ms; // reset gri_ms
}

static void reset_pace_flag(ChannelData *ch_data)
{
	pthread_mutex_lock(g_shared_data.mutex);
	ch_data->pace_flag = 0; // Reset the pace state
	pthread_mutex_unlock(g_shared_data.mutex);
}

static void set_pace_flag(ChannelData *ch_data, int pace_flag)
{
	pthread_mutex_lock(g_shared_data.mutex);
	ch_data->pace_flag = pace_flag; // Set the pace state
	pthread_mutex_unlock(g_shared_data.mutex);
}

static void calculate_threshold(ChannelData *ch_data)
{
	// get mean of the lowest slope values
	ch_data->threshold = (ch_data->lsv_sum / ch_data->lsv_count) * 4.5; // %%%%%% why 4.5?
	ch_data->threshold_flag = 1;																				// Set the threshold flag to indicate that threshold is calculated
}

int run_pacemaker(PacemakerData *p_data, ChannelData *ch_data, void (*callback_unlock_mutex)(void))
{
	// mutex is locked at this point, hence the mutex callback function is passed to this function to unlock it after processing
	int timer_ms = ch_data->ch_rb_ptr->cur_time_ms; // Get the current timer value
	int pace_flag_snapshot = ch_data->pace_flag;		// Get snapshot of the current pace state
	// Take a snapshot of the ring buffer and timer_ms

	if (!rb_snapshot(ch_data->ch_rb_ptr, snapshot_buffer, g_buffer_offset))
	{
		printf("\nError taking snapshot of ring buffer.\n");
		callback_unlock_mutex();
		return 1; // Return error
	}
	callback_unlock_mutex(); // Unlock the mutex after taking the snapshot

	// Phase 1 : recording the lowest slope values
	if (timer_ms < (p_data->learn_time_ms))
	{
		fflush(stdout); // Print count and clear leftovers
		double lowest_slope = get_lowest_scope(ch_data, snapshot_buffer);
		// Get the execution time

		// Set the pacemaker state to learning if not already set
		if (ch_data->pm_state != PM_LEARNING)
		{
			ch_data->pm_state = PM_LEARNING;
		}

		// Log the execution time and worst-case execution time
		if (logging_enabled)
		{
			// Extract the execution time
			get_n_set_execution_time(ch_data->et_timer_ptr);
			printf("\r[%.2fs][WCET%.2fms][ET%.2fms] Learning slope values...", (float)timer_ms / 1000.0f, ch_data->et_timer_ptr->wcet, ch_data->et_timer_ptr->et);
			fflush(stdout); // Print count and clear leftovers
		}

		// printf("\t[Buff-%.15f] Lowest s//lope : %f\n", (float)timer_ms / (float)1000, lowest_slope);
		// printf("%f\n", lowest_slope);

		ch_data->lsv_sum += lowest_slope;
		// printf("\t\tLowest slope sum : %f\n", shared_data->lsv_sum);
		ch_data->lsv_count++;
	}
	else
	{
		if (!ch_data->threshold_flag)
		{
			// Phase 2 : Calculating the threshold value from the lowest slope values
			calculate_threshold(ch_data);

			// Set the pacemaker state to learning if not already set
			if (ch_data->pm_state != PM_LEARNING)
			{
				ch_data->pm_state = PM_LEARNING;
			}

			if (logging_enabled)
			{
				// Get the execution time
				get_n_set_execution_time(ch_data->et_timer_ptr);
				printf("\n\t\tActivation detection threshold is: %f\n\n", ch_data->threshold); // %%%%%% why 4.5?
			}
		}
		else
		{
			// Phase 3 : Pacing decision making
			double lowest_slope = get_lowest_scope(ch_data, snapshot_buffer);

			// Senario 1 : No detection yet AND (within the lri threshold OR pace_flag is 2)
			if (ch_data->activation_flag == 0 && (ch_data->lri_ms <= p_data->lri_thresh_ms || pace_flag_snapshot == 2))
			{
				// Set the pacemaker state to detecting(all paths within this scope is detection state)
				if (ch_data->pm_state != PM_DETECTING)
				{
					ch_data->pm_state = PM_DETECTING;
				}

				if (logging_enabled)
				{
					// Get the execution time
					get_n_set_execution_time(ch_data->et_timer_ptr);
				}
				// Senario 1.1 : Activation detected within the LRI threshold OR when there was a pacing
				if (lowest_slope < ch_data->threshold)
				{

					if (logging_enabled)
					{
						print_detection(ch_data, timer_ms, 1, pace_flag_snapshot); // Print activation detection
						printf("\n\t\t\t\tResetting timers and pace flag.\n");
						printf("\t\t\t\tActivation flag ENABLED!\n");
					}

					reset_timers(ch_data, p_data); // Reset the LRI and GRI timers
					reset_pace_flag(ch_data);			 // Reset the pace state
					// shared_data->start = 1;
					ch_data->activation_flag = 1;
				}
				// Senario 1.2 : No activation yet (within the LRI threshold OR when there was a pacing)
				else
				{
					ch_data->lri_ms += g_samp_interval_ms; // increment lri_ms by samp interval
					ch_data->gri_ms -= g_samp_interval_ms; // decrement gri_ms by samp interval
					if (logging_enabled)
					{
						print_waiting(ch_data, timer_ms, 1, 0); // Print animation
					}
				}
			}
			// Senario 2 : after activation AND (within the lri threshold OR pace_flag is 2)
			else if (ch_data->activation_flag != 0 && (ch_data->lri_ms <= p_data->lri_thresh_ms || pace_flag_snapshot == 2))
			{
				if (logging_enabled)
				{
					// Get the execution time
					get_n_set_execution_time(ch_data->et_timer_ptr);
				}
				// Senario 2.1 : GRI threshold is exceeded AND there was a activation detected
				// Reset the LRI and GRI timers
				if (lowest_slope < ch_data->threshold && ch_data->gri_ms <= 0)
				{
					// Set the pacemaker state to detecting if not already set
					if (ch_data->pm_state != PM_DETECTING)
					{
						ch_data->pm_state = PM_DETECTING;
					}

					if (logging_enabled)
					{
						print_detection(ch_data, timer_ms, 1, pace_flag_snapshot); // Print activation detection
						printf("\n\t\t\t\tlowest_slope: %f / threshold: %f\n", lowest_slope, ch_data->threshold);
						printf("\t\t\t\tResetting timers and pace flag.\n");
					}

					reset_timers(ch_data, p_data); // Reset the LRI and GRI timers
					reset_pace_flag(ch_data);			 // Reset the pace state
				}
				// Senario 2.2 : Ignoring and not ignoring within and after the GRI threshold
				else
				{
					if (ch_data->pm_state != PM_DETECTING)
					{
						ch_data->pm_state = PM_DETECTING;
					}
					// Senario 2.2.1 : No activation , but GRI threshold is exceeded
					if (ch_data->gri_ms <= 0)
					{

						if (logging_enabled)
						{
							print_waiting(ch_data, timer_ms, 1, 0); // Print animation
						}
					}
					// Senario 2.2.2 : Activation happened but within the GRI threshold
					else if (lowest_slope < ch_data->threshold)
					{
						if (ch_data->pm_state != PM_IGNORING)
						{
							ch_data->pm_state = PM_IGNORING; // Set the pacemaker state to Ignore
						}
						if (logging_enabled)
						{
							print_waiting(ch_data, timer_ms, 0, 1); // Print animation

							// printf("\n\t\tlowest_slope: %f / threshold: %f\n", lowest_slope, ch_data->threshold);

							// print_waiting(ch_data, timer_ms); // Print animation
							// printf("\t\tWithin GRI, IGNORING!\n");
						}
					}
					// Senario 2.2.3 : no activation and GRI threshold is not exceeded
					else
					{
						if (ch_data->pm_state != PM_IGNORING)
						{
							ch_data->pm_state = PM_IGNORING; // Set the pacemaker state to Ignore
						}
						if (logging_enabled)
						{
							// printf("\r%sIGNORING at %.2f secs. ACT %d / LRI %d / GRI %d / PACE %d", RT_TITLE, (float)timer_ms / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_flag);
							// fflush(stdout); // Flush the output to show the animation

							print_waiting(ch_data, timer_ms, 0, 0); // Print animation
						}
					}

					ch_data->lri_ms += g_samp_interval_ms; // increment lri_ms by samp interval
					ch_data->gri_ms -= g_samp_interval_ms; // decrement gri_ms by samp interval
				}
			}
			else
			{
				// Set the pacemaker state to Ignore if not already set
				if (ch_data->pm_state != PM_PACING)
				{
					ch_data->pm_state = PM_PACING;
				}
				// Get the execution time
				// Senario 3 : Exceeded the LRI threshold
				set_pace_flag(ch_data, 2); // Reset the pace state
				if (logging_enabled)
				{
					get_n_set_execution_time(ch_data->et_timer_ptr);
					print_pacing(ch_data, timer_ms); // Print activation detection

					// printf("\n%sPACING since No activation after LRI at %.2f seconds.\n\t\tACT %d\n\t\tLRI %d.\n\t\tGRI %d\n\t\tPACE %d\n", RT_TITLE, (float)timer_ms / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_flag);
				}
			}
		}
	}

	return 0; // Return success
}
