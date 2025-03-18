#ifndef ACTIVATION_DETECTION_H
#define ACTIVATION_DETECTION_H

#include <stdio.h> // for printf (if desired)
#include "config.h"

/**
 * @brief Apply Nonlinear Energy Operator (NEO) transform to a signal
 *
 * The NEO transform is defined as: y[n] = x[n]^2 - x[n-1]*x[n+1]
 * This is useful for detecting transient events like neural spikes.
 *
 * @param in_signal Pointer to the input signal array
 * @param in_signal_len Length of the input signal
 * @param out_signal Pointer to the output signal array
 * @param out_signal_len Length of the output signal
 * @return 0 on success, 1 on error
 */
int neo_transform(double *in_signal, int in_signal_len, double *out_signal, int out_signal_len);

int moving_average_filtering(double *input_signal, double *output_signal, int length, int sample_rate);

#endif