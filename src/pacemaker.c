#include "global.h"
#include "pacemaker.h"
#include <stdio.h>

#define STR_WIDTH 25
double snapshot_buffer[ACTIVATION_DETECTION_BUFFER_SIZE]; // Buffer to hold the snapshot of the ring buffer

static void print_waiting(ChannelData *ch_data, int timer, int waiting, int detection)
{
	const char *str;
	if (!waiting)
	{
		if (detection)
			str = "DETECTION IGNORED";
		else
			str = "Ignoring...";
	}
	else
	{
		str = "Waiting...";
	}
	printf("\r%s[%.2fsecs] %-*s ACT %d / LRI %d / GRI %d / PACE %d",
				 RT_TITLE, (float)timer / 1000.0f, STR_WIDTH, str,
				 ch_data->activation_flag, ch_data->lri_ms,
				 ch_data->gri_ms, ch_data->pace_state);
	// printf("\r%s[%.2fsecs] %s ACT %d / LRI %d / GRI %d / PACE %d", RT_TITLE, (float)timer / (float)1000, str, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_state);
	// if (!waiting)
	// {
	// 	if (detection)
	// 		printf("\n");
	// }

	if (!detection)
		fflush(stdout); // Print count and clear leftovers

	// printf("\r%sWaiting at %.2f secs. ACT %d / LRI %d / GRI %d / PACE %d", RT_TITLE, (float)timer / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_state);
	// fflush(stdout); // Print count and clear leftovers
}

static void print_detection(ChannelData *ch_data, int timer, int detection, int pacing)
{
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

	printf("\r%s[%.2fsecs] %-*s ACT %d / LRI %d / GRI %d / PACE %d",
				 RT_TITLE, (float)timer / 1000.0f, STR_WIDTH, str,
				 ch_data->activation_flag, ch_data->lri_ms,
				 ch_data->gri_ms, ch_data->pace_state);
	fflush(stdout); // Print count and clear leftovers

	// printf("\n%sActivation detected at %.2f seconds.\n\t\t\t\tACT %d\n\t\tLRI %d.\n\t\tGRI %d\n\t\tPACE %d\n", RT_TITLE, (float)timer / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_state);
}
static void print_pacing(ChannelData *ch_data, int timer)
{
	char *str = "PACING!";

	printf("\n%s[%.2fsecs] %-*s ACT %d / LRI %d / GRI %d / PACE %d\n",
				 RT_TITLE, (float)timer / 1000.0f, STR_WIDTH, str,
				 ch_data->activation_flag, ch_data->lri_ms,
				 ch_data->gri_ms, ch_data->pace_state);

	// printf("\n%sActivation detected at %.2f seconds.\n\t\t\t\tACT %d\n\t\tLRI %d.\n\t\tGRI %d\n\t\tPACE %d\n", RT_TITLE, (float)timer / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_state);
}

static double get_lowest_scope(ChannelData *ch_data, double *buffer)
{
	double lowest_slope = (buffer[1] - buffer[0]) / ((double)g_samp_interval_ms / 1000); // initial slope set
	for (int i = 2; i < ch_data->rb->size; i++)
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

static void reset_pace_state(ChannelData *ch_data)
{
	pthread_mutex_lock(g_shared_data.mutex);
	ch_data->pace_state = 0; // Reset the pace state
	pthread_mutex_unlock(g_shared_data.mutex);
}

static void set_pace_state(ChannelData *ch_data, int pace_state)
{
	pthread_mutex_lock(g_shared_data.mutex);
	ch_data->pace_state = pace_state; // Set the pace state
	pthread_mutex_unlock(g_shared_data.mutex);
}

static void calculate_threshold(ChannelData *ch_data)
{
	// get mean of the lowest slope values
	ch_data->threshold = (ch_data->lsv_sum / ch_data->lsv_count) * 4.5; // %%%%%% why 4.5?
	ch_data->threshold_flag = 1;																				// Set the threshold flag to indicate that threshold is calculated
}

int run_pacemaker(PacemakerData *p_data, ChannelData *ch_data, int *timer, void (*callback_unlock_mutex)(void))
{
	// mutex is locked at this point, hence the mutex callback function is passed to this function to unlock it after processing
	int timer_ms = *timer;												 // Get the current timer value
	int pace_state_snapshot = ch_data->pace_state; // Get snapshot of the current pace state
	// Take a snapshot of the ring buffer and timer_ms

	if (!rb_snapshot(ch_data->rb, snapshot_buffer, g_buffer_offset))
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
		printf("\r%s[%.2fsecs] Learning slope values...", PT_TITLE, (float)timer_ms / (float)1000);
		fflush(stdout); // Print count and clear leftovers
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
			printf("\n%sActivation detection threshold is: %f\n\n", PT_TITLE, ch_data->threshold); // %%%%%% why 4.5?
		}
		else
		{
			// Phase 3 : Pacing decision making
			double lowest_slope = get_lowest_scope(ch_data, snapshot_buffer);

			// Senario 1 : No detection yet AND (within the lri threshold OR pace_state is 2)
			if (ch_data->activation_flag == 0 && (ch_data->lri_ms <= p_data->lri_thresh_ms || pace_state_snapshot == 2))
			{
				// Senario 1.1 : Activation detected within the LRI threshold OR when there was a pacing
				if (lowest_slope < ch_data->threshold)
				{
					print_detection(ch_data, timer_ms, 1, pace_state_snapshot); // Print activation detection
					printf("\n\t\tResetting timers and pace flag.\n");
					printf("\t\tActivation flag ENABLED!\n");

					reset_timers(ch_data, p_data); // Reset the LRI and GRI timers
					reset_pace_state(ch_data);		 // Reset the pace state
					// shared_data->start = 1;
					ch_data->activation_flag = 1;
				}
				// Senario 1.2 : No activation yet (within the LRI threshold OR when there was a pacing)
				else
				{
					ch_data->lri_ms += g_samp_interval_ms; // increment lri_ms by samp interval
					ch_data->gri_ms -= g_samp_interval_ms; // decrement gri_ms by samp interval

					print_waiting(ch_data, timer_ms, 1, 0); // Print animation
				}
			}
			// Senario 2 : after activation AND (within the lri threshold OR pace_state is 2)
			else if (ch_data->activation_flag != 0 && (ch_data->lri_ms <= p_data->lri_thresh_ms || pace_state_snapshot == 2))
			{
				// Senario 2.1 : GRI threshold is exceeded AND there was a activation detected
				// Reset the LRI and GRI timers
				if (lowest_slope < ch_data->threshold && ch_data->gri_ms <= 0)
				{
					print_detection(ch_data, timer_ms, 1, pace_state_snapshot); // Print activation detection
					printf("\n\t\tlowest_slope: %f / threshold: %f\n", lowest_slope, ch_data->threshold);
					printf("\t\tResetting timers and pace flag.\n");

					reset_timers(ch_data, p_data); // Reset the LRI and GRI timers
					reset_pace_state(ch_data);		 // Reset the pace state
				}
				// Senario 2.2 : Ignoring activations within the GRI threshold
				else
				{
					// Senario 2.2.1 : No activation , but GRI threshold is exceeded
					if (ch_data->gri_ms <= 0)
					{
						print_waiting(ch_data, timer_ms, 1, 0); // Print animation
					}
					// Senario 2.2.2 : Activation happened but within the GRI threshold
					else if (lowest_slope < ch_data->threshold)
					{
						print_waiting(ch_data, timer_ms, 0, 1); // Print animation

						// printf("\n\t\tlowest_slope: %f / threshold: %f\n", lowest_slope, ch_data->threshold);

						// print_waiting(ch_data, timer_ms); // Print animation
						// printf("\t\tWithin GRI, IGNORING!\n");
					}
					// Senario 2.2.3 : no activation and GRI threshold is not exceeded
					else
					{
						// printf("\r%sIGNORING at %.2f secs. ACT %d / LRI %d / GRI %d / PACE %d", RT_TITLE, (float)timer_ms / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_state);
						// fflush(stdout); // Flush the output to show the animation

						print_waiting(ch_data, timer_ms, 0, 0); // Print animation
					}

					ch_data->lri_ms += g_samp_interval_ms; // increment lri_ms by samp interval
					ch_data->gri_ms -= g_samp_interval_ms; // decrement gri_ms by samp interval
				}
			}
			// Senario 3 : Exceeded the LRI threshold
			else
			{
				set_pace_state(ch_data, 2);			 // Reset the pace state
				print_pacing(ch_data, timer_ms); // Print activation detection

				// printf("\n%sPACING since No activation after LRI at %.2f seconds.\n\t\tACT %d\n\t\tLRI %d.\n\t\tGRI %d\n\t\tPACE %d\n", RT_TITLE, (float)timer_ms / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_state);
			}
		}
	}

	return 0; // Return success
}
