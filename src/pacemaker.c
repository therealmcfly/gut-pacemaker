#include "pacemaker.h"
#include "shared_data.h"
#include <stdio.h>

double snapshot_buffer[ACTIVATION_DETECTION_BUFFER_SIZE]; // Buffer to hold the snapshot of the ring buffer

static void print_waiting(SharedData *shared_data, int timer)
{
	printf("\r%sWaiting at %.2f secs. ACT %d / LRI %d / GRI %d / PACE %d", RT_TITLE, (float)timer / (float)1000, shared_data->activation_flag, shared_data->lri_ms, shared_data->gri_ms, shared_data->pace_flag);
	fflush(stdout); // Print count and clear leftovers
}
static void print_detection(SharedData *shared_data, int timer)
{
	printf("\n%sActivation detected at %.2f seconds.\n\t\tACT %d\n\t\tLRI %d.\n\t\tGRI %d\n\t\tPACE %d\n", RT_TITLE, (float)timer / (float)1000, shared_data->activation_flag, shared_data->lri_ms, shared_data->gri_ms, shared_data->pace_flag);
}

static double get_lowest_scope(SharedData *shared_data, double *buffer)
{
	double lowest_slope = (buffer[1] - buffer[0]) / ((double)shared_data->samp_interval_ms / 1000); // initial slope set
	for (int i = 2; i < shared_data->buffer->size; i++)
	{
		// slope = (y2 - y1) / (x2 - x1) = (current sample - prev sample) / samp_interval_ms
		double slope = (buffer[i] - buffer[i - 1]) / ((double)shared_data->samp_interval_ms / 1000);

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

static void calculate_threshold(SharedData *shared_data)
{
	// get mean of the lowest slope values
	shared_data->threshold = (shared_data->lsv_sum / shared_data->lsv_count) * 4.5; // %%%%%% why 4.5?
	shared_data->threshold_flag = 1;																								// Set the threshold flag to indicate that threshold is calculated
}

int run_pacemaker(SharedData *shared_data, void (*callback_unlock_mutex)(void))
{
	// printf("\n%sRunning pacemaker for buffer %d at %.2f seconds...\n", RT_TITLE, shared_data->buffer_count + 1, *shared_data->timer_ms);
	// Take a snapshot of the ring buffer and timer_ms
	int timer_ms = *shared_data->timer_ms;

	if (!rb_snapshot(shared_data->buffer, snapshot_buffer, shared_data->buff_offset))
	{
		printf("\nError taking snapshot of ring buffer.\n");
		callback_unlock_mutex();
		return 1; // Return error
	}

	// if (*shared_data->timer_ms > 1.40 && *shared_data->timer_ms < 1.50)
	// {
	// 	for (int j = 0; j < shared_data->buffer->size; j++)
	// 	{
	// 		printf("[Buff-%.2f][%d]: %.15f\n", timer_ms, j, snapshot_buffer[j]);
	// 	}
	// }
	callback_unlock_mutex(); // Unlock the mutex after taking the snapshot

	// Phase 1 : recording the lowest slope values
	if (timer_ms < (shared_data->learn_time_ms))
	{
		double lowest_slope = get_lowest_scope(shared_data, snapshot_buffer);
		// printf("\t[Buff-%.15f] Lowest slope : %f\n", (float)timer_ms / (float)1000, lowest_slope);
		// printf("%f\n", lowest_slope);

		shared_data->lsv_sum += lowest_slope;
		// printf("\t\tLowest slope sum : %f\n", shared_data->lsv_sum);
		shared_data->lsv_count++;
	}
	else
	{
		if (!shared_data->threshold_flag)
		{
			// Phase 2 : Calculating the threshold value from the lowest slope values
			calculate_threshold(shared_data);
			// printf("\tTimer: %.2f seconds\n", (float)timer_ms / (float)1000);
			// printf("\tLowest slope values sum: %f\n", shared_data->lsv_sum);
			// printf("\tLowest slope values count: %d\n", shared_data->lsv_count);
			// printf("\tMean: %f\n\n", shared_data->lsv_sum / shared_data->lsv_count);
			printf("\tThreshold: %f\n\n", shared_data->threshold); // %%%%%% why 4.5?
		}
		else
		{
			// Phase 3 : Pacing decision making
			double lowest_slope = get_lowest_scope(shared_data, snapshot_buffer);

			// Senario 1 : No detection yet AND (within the lri threshold OR pace_flag is 2)
			if (shared_data->activation_flag == 0 && (shared_data->lri_ms <= shared_data->lri_thresh_ms || shared_data->pace_flag == 2))
			{
				// Senario 1.1 : Activation detected (within the LRI threshold OR when there was a pacing)
				if (lowest_slope < shared_data->threshold)
				{
					print_detection(shared_data, timer_ms); // Print activation detection
					printf("\t\tResetting timers and pace flag.\n");
					printf("\t\tActivation flag ENABLED!\n");

					shared_data->lri_ms = 0;													// reset lri_ms
					shared_data->gri_ms = shared_data->gri_thresh_ms; // reset gri_ms
					shared_data->pace_flag = 0;
					// shared_data->start = 1;
					shared_data->activation_flag = 1;
				}
				// Senario 1.2 : No activation yet (within the LRI threshold OR when there was a pacing)
				else
				{
					shared_data->lri_ms += shared_data->samp_interval_ms; // increment lri_ms by samp interval
					shared_data->gri_ms -= shared_data->samp_interval_ms; // decrement gri_ms by samp interval

					print_waiting(shared_data, timer_ms); // Print animation
				}
			}
			// Senario 2 : after activation AND (within the lri threshold OR pace_flag is 2)
			else if (shared_data->activation_flag != 0 && (shared_data->lri_ms <= shared_data->lri_thresh_ms || shared_data->pace_flag == 2))
			{
				// Senario 2.1 : GRI threshold is exceeded AND there was a activation detected
				// Reset the LRI and GRI timers
				if (lowest_slope < shared_data->threshold && shared_data->gri_ms <= 0)
				{
					print_detection(shared_data, timer_ms); // Print activation detection
					printf("\t\tResetting timers and pace flag.\n");

					shared_data->lri_ms = 0;													// reset lri_ms
					shared_data->gri_ms = shared_data->gri_thresh_ms; // reset gri_ms
					shared_data->pace_flag = 0;
				}
				// Senario 2.2 : Ignoring activations within the GRI threshold
				else
				{
					if (shared_data->gri_ms <= 0)
					{
						print_waiting(shared_data, timer_ms); // Print animation
					}
					else if (lowest_slope < shared_data->threshold)
					{
						print_waiting(shared_data, timer_ms); // Print animation
						printf("\t\tWithin GRI, IGNORING!\n");
					}
					else
					{
						printf("\r%sIGNORING at %.2f secs. ACT %d / LRI %d / GRI %d / PACE %d", RT_TITLE, (float)timer_ms / (float)1000, shared_data->activation_flag, shared_data->lri_ms, shared_data->gri_ms, shared_data->pace_flag);
						fflush(stdout); // Flush the output to show the animation
					}

					shared_data->lri_ms += shared_data->samp_interval_ms; // increment lri_ms by samp interval
					shared_data->gri_ms -= shared_data->samp_interval_ms; // decrement gri_ms by samp interval
				}
			}
			// Senario 3 : Exceeded the LRI threshold
			else
			{
				printf("\n%sPACE! No activation after LRI at %.2f seconds.\n\t\tACT %d\n\t\tLRI %d.\n\t\tGRI %d\n\t\tPACE %d\n", RT_TITLE, (float)timer_ms / (float)1000, shared_data->activation_flag, shared_data->lri_ms, shared_data->gri_ms, shared_data->pace_flag);
				printf("\t\tPACE!\n");

				shared_data->pace_flag = 2; // Enable pacing flag
			}
		}
	}

	return 0; // Return success
}
