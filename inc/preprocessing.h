#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include <stdio.h>	// for printf (if desired)
#include <stdlib.h> // for malloc/free (if allowed in your environment)
#include "config.h"

int lowpass_filter(double *in_signal, double *lpf_signal, int signal_length);
int lowpass_fir_filter(const double *coeffs, int num_coeffs, const double *in_signal, double *out_signal, int signal_length);
int highpass_filter(double *in_signal, int in_signal_len, double *out_hpf_signal, int *out_hpf_signal_len);
int highpass_fir_filter(const double *coeffs, int num_coeffs, const double *in_signal, int in_signal_len, double *out_signal, int out_signal_len);
int apply_padding(double *in_signal, int in_signal_len, double *out_padded_signal, int out_padded_signal_len, int padding_size);
int remove_artifacts(const double *in_signal, double *out_signal, int signal_length);
int detect_artifact(const double *window, int window_size, double threshold);
void remove_artifact(double *window, int window_size, int x1, int x2);
#endif