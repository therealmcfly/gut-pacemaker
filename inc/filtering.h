#ifndef FILTERING_H
#define FILTERING_H

#include <stdio.h> // for printf (if desired)
#include "config.h"

int lowpass_filter(double *in_signal, double *lpf_signal, int signal_length);
int fir_filter(const double *coeffs, int num_coeffs, const double *in_signal,
							 double *out_signal, int signal_length);
#endif // FILTERING_H