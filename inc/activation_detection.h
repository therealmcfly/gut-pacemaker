#ifndef ACTIVATION_DETECTION_H
#define ACTIVATION_DETECTION_H

#include <stdio.h> // for printf (if desired)
#include "config.h"

int neo_transform(double *in_signal, int in_signal_len, double *out_signal, int out_signal_len);
int moving_average_filter(double *input_signal, double *output_signal, int signal_length, int window_size);

#endif