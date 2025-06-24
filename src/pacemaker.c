#include "global.h"
#include "pacemaker.h"
#include <stdio.h>

double snapshot_buffer[ACTIVATION_DETECTION_BUFFER_SIZE]; // Buffer to hold the snapshot of the ring buffer

static void print_waiting(ChannelData *ch_data, int timer)
{
	printf("\r%sWaiting at %.2f secs. ACT %d / LRI %d / GRI %d / PACE %d", RT_TITLE, (float)timer / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_flag);
	fflush(stdout); // Print count and clear leftovers
}
static void print_detection(ChannelData *ch_data, int timer)
{
	printf("\n%sActivation detected at %.2f seconds.\n\t\tACT %d\n\t\tLRI %d.\n\t\tGRI %d\n\t\tPACE %d\n", RT_TITLE, (float)timer / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_flag);
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

static void calculate_threshold(ChannelData *ch_data)
{
	// get mean of the lowest slope values
	ch_data->threshold = (ch_data->lsv_sum / ch_data->lsv_count) * 4.5; // %%%%%% why 4.5?
	ch_data->threshold_flag = 1;																				// Set the threshold flag to indicate that threshold is calculated
}

int run_pacemaker(PacemakerData *p_data, ChannelData *ch_data, int *timer, void (*callback_unlock_mutex)(void))
{
	int timer_ms = *timer; // Get the current timer value
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
		double lowest_slope = get_lowest_scope(ch_data, snapshot_buffer);
		// printf("\t[Buff-%.15f] Lowest slope : %f\n", (float)timer_ms / (float)1000, lowest_slope);
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
			printf("\tThreshold: %f\n\n", ch_data->threshold); // %%%%%% why 4.5?
		}
		else
		{
			// Phase 3 : Pacing decision making
			double lowest_slope = get_lowest_scope(ch_data, snapshot_buffer);

			// Senario 1 : No detection yet AND (within the lri threshold OR pace_flag is 2)
			if (ch_data->activation_flag == 0 && (ch_data->lri_ms <= p_data->lri_thresh_ms || ch_data->pace_flag == 2))
			{
				// Senario 1.1 : Activation detected (within the LRI threshold OR when there was a pacing)
				if (lowest_slope < ch_data->threshold)
				{
					print_detection(ch_data, timer_ms); // Print activation detection
					printf("\t\tResetting timers and pace flag.\n");
					printf("\t\tActivation flag ENABLED!\n");

					ch_data->lri_ms = 0;										 // reset lri_ms
					ch_data->gri_ms = p_data->gri_thresh_ms; // reset gri_ms
					ch_data->pace_flag = 0;
					// shared_data->start = 1;
					ch_data->activation_flag = 1;
				}
				// Senario 1.2 : No activation yet (within the LRI threshold OR when there was a pacing)
				else
				{
					ch_data->lri_ms += g_samp_interval_ms; // increment lri_ms by samp interval
					ch_data->gri_ms -= g_samp_interval_ms; // decrement gri_ms by samp interval

					print_waiting(ch_data, timer_ms); // Print animation
				}
			}
			// Senario 2 : after activation AND (within the lri threshold OR pace_flag is 2)
			else if (ch_data->activation_flag != 0 && (ch_data->lri_ms <= p_data->lri_thresh_ms || ch_data->pace_flag == 2))
			{
				// Senario 2.1 : GRI threshold is exceeded AND there was a activation detected
				// Reset the LRI and GRI timers
				if (lowest_slope < ch_data->threshold && ch_data->gri_ms <= 0)
				{
					print_detection(ch_data, timer_ms); // Print activation detection
					printf("\t\tResetting timers and pace flag.\n");

					ch_data->lri_ms = 0;										 // reset lri_ms
					ch_data->gri_ms = p_data->gri_thresh_ms; // reset gri_ms
					ch_data->pace_flag = 0;
				}
				// Senario 2.2 : Ignoring activations within the GRI threshold
				else
				{
					if (ch_data->gri_ms <= 0)
					{
						print_waiting(ch_data, timer_ms); // Print animation
					}
					else if (lowest_slope < ch_data->threshold)
					{
						print_waiting(ch_data, timer_ms); // Print animation
						printf("\t\tWithin GRI, IGNORING!\n");
					}
					else
					{
						printf("\r%sIGNORING at %.2f secs. ACT %d / LRI %d / GRI %d / PACE %d", RT_TITLE, (float)timer_ms / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_flag);
						fflush(stdout); // Flush the output to show the animation
					}

					ch_data->lri_ms += g_samp_interval_ms; // increment lri_ms by samp interval
					ch_data->gri_ms -= g_samp_interval_ms; // decrement gri_ms by samp interval
				}
			}
			// Senario 3 : Exceeded the LRI threshold
			else
			{
				printf("\n%sPACE! No activation after LRI at %.2f seconds.\n\t\tACT %d\n\t\tLRI %d.\n\t\tGRI %d\n\t\tPACE %d\n", RT_TITLE, (float)timer_ms / (float)1000, ch_data->activation_flag, ch_data->lri_ms, ch_data->gri_ms, ch_data->pace_flag);
				printf("\t\tPACE!\n");

				ch_data->pace_flag = 2; // Enable pacing flag
			}
		}
	}

	return 0; // Return success
}
