#ifndef FILTERING_H
#define FILTERING_H

#include <stdio.h> // for printf (if desired)
#include "config.h"

int lowpass_filter(double *in_signal, double *lpf_signal, int signal_length);
int lowpass_fir_filter(const double *coeffs, int num_coeffs, const double *in_signal, double *out_signal, int signal_length);
int highpass_filter(double *in_signal, double *hpf_signal, int signal_length);
int highpass_fir_filter(const double *coeffs, int num_coeffs, const double *in_signal, double *out_signal, int signal_length);
void apply_padding(double *in_signal, int in_signal_len, double *padded_signal, int padded_signal_len, int padding_size);
#endif // FILTERING_H