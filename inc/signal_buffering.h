#ifndef S_BUFFERING_H
#define S_BUFFERING_H

#include <stdio.h>	// for printf (if desired)
#include <stdlib.h> // for malloc/free (if allowed in your environment)

#include "config.h"
#include "preprocessing.h"
#include "result_check.h"
#include "detection.h"

#define SUCCESS 0
#define ERROR 1
#define ERROR_BUFFER_SIZE 2

int signal_buffering(double *in_signal, size_t signal_length, int *channel_num, char *file_name, int *cur_data_freq);

#endif