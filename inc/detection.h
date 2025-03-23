#ifndef DETECTION_H
#define DETECTION_H

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

int moving_average_filtering(double *in_signal, double *out_signal, int out_signal_len, int sample_rate);

int edge_detection(const double *in_processed_signal, int in_processed_sig_len, const double *in_neo_signal, int in_neo_sig_len, double *out_ed_signal, int out_ed_signal_len);

void conv_1d_same(const double *input, int input_size, const double *kernel, int kernel_size, double *output);

int detect_activation(double *in_ed_signal, int in_ed_signal_len, int *out_activation_indices, int *out_num_activation, int cur_buffer_start_index);

void cleanup_activation_locs(int *locs, int *locs_len, int signal_length, int threshold);

#endif