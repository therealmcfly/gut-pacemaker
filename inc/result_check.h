#ifndef VALIDATION_H
#define VALIDATION_H

#include <stddef.h> // For size_t

int verify_signals(double *signal, size_t num_of_data, double **verify_data, size_t *verify_num_rows, size_t *verify_num_cols);

#endif
