#ifndef S_PROCESSING_H
#define S_PROCESSING_H

#include <stdio.h>	// for printf (if desired)
#include <stdlib.h> // for malloc/free (if allowed in your environment)
#include "config.h"
#include "filtering.h"

// main signal processing function
#define SUCCESS 0
#define ERROR 1
#define ERROR_BUFFER_SIZE 2

int signal_buffering(double *inSignal, size_t signal_length, int *channel_num);

#endif // S_PROCESSING_H