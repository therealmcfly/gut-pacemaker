#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include <stdlib.h> // for malloc/free (if allowed in your environment)

int detect_activations(double *in_signal, size_t signal_length, int *channel_num, char *file_name, int *cur_data_freq);
int detection_pipeline(double *buffer, int shift, int i, int *num_activations, int *activations);
int processing_pipeline(int shift, int i, int *num_activations, int *activations, void (*callback_unlock_mutex)(void));
#endif