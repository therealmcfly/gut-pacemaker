#ifndef RESULT_CHECK_H
#define RESULT_CHECK_H

#include <stddef.h> // For size_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "file_io.h"

int check_processing_result(double *signal, size_t signal_length, int channel_num, char *file_name, char *ver_code, int shift);

#endif
