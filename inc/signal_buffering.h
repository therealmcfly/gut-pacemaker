#ifndef S_PROCESSING_H
#define S_PROCESSING_H

#include <stdio.h>	// for printf (if desired)
#include <stdlib.h> // for malloc/free (if allowed in your environment)
#include "config.h"
#include "filtering.h"

#define FILTER_ORDER 50

void signal_buffering(float *inSignal, size_t signal_length);
int load_coeffs(char *filename, float *lowpass_coeffs);

#endif // S_PROCESSING_H