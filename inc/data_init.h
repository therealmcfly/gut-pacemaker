#ifndef DATA_INIT_H
#define DATA_INIT_H

#include <stddef.h> // for size_t

#define MAX_CHANNEL 2000
#define MATLAB_DIRECTORY "MATLAB Model/"
#define TARGET_FREQUENCY 32 // Signal frequency in Hz

float *get_sample_data(int user_argc, char *user_argv[], size_t *out_data_length);

#endif
