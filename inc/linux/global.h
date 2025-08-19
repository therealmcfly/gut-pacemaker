#ifndef GLOBAL_H
#define GLOBAL_H

#include "shared_data.h"
#include "config.h"
#include "timer_util.h"

// Global vars (for static dataset mode, can remove for embedded implimentaion)
extern size_t signal_length;
extern int channel_num;
extern char file_name[100];
extern int cur_data_freq;

// not for static dataset mode(dont remove in embedded implimentation)
extern int g_samp_interval_ms; // Sampling interval in milliseconds
extern int g_buffer_offset;		 // Overlap count for ring buffer

extern SharedData g_shared_data; // Global shared data for all threads

extern int logging_enabled; // Flag to enable/disable logging

#endif
