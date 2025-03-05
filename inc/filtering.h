#ifndef FILTERING_H
#define FILTERING_H

#include <stdio.h> // for printf (if desired)
#include "config.h"

void lowpass_filter(float *in_signal, float *lpf_signal, int signal_length, float *lpf_coeffs, int coeff_length);

#endif // FILTERING_H