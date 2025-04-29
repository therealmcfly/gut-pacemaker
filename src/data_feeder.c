#include "data_feeder.h"

int data_feeder(double *signals, int signal_len)
{
	clock_t start = clock();
	double next_tick = 0.0;

	for (int i = 0; i < signal_len; i++)
	{
		// Wait until the next tick
		while (((double)(clock() - start) / CLOCKS_PER_SEC) < next_tick)
		{
			// Busy wait
		}

		// Feed the sample into your processing pipeline
		float sample = signals[i];
		printf("Sample %d: %f\n", i, sample);

		// Schedule next tick
		next_tick += SAMPLING_INTERVAL_SEC / SIMULATION_SPEEDUP;
	}
	return 0;
}