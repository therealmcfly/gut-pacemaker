#ifndef DATA_FEEDER_H
#define DATA_FEEDER_H
#include <time.h>
#include <stdio.h>

#define SAMPLING_FREQ_HZ 32
#define SAMPLING_INTERVAL_SEC (1.0 / SAMPLING_FREQ_HZ) // = 0.03125
#define SIMULATION_SPEEDUP 1.0												 // >1.0 to speed up simulation

#define REALTIME_MODE 1

int data_feeder(double *signals, int signal_len);

#endif