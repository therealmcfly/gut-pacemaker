#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h> // for NULL
#include "config.h"

// Circular buffer size
#define C_BUFFER_SIZE 10 // Must be a multiple of 2

typedef struct
{
	double buffer[C_BUFFER_SIZE]; // Fixed-size buffer
	double *head;									// Write pointer
	double *tail;									// Read pointer
	double *end;									// Pointer to buffer + BUFFER_SIZE
	int is_full;									// Flag to indicate if the buffer is full
} CircularBufferDouble;

// Initializes the buffer
void cb_init(CircularBufferDouble *cb);

// Returns true if the buffer is full
bool cb_is_full(CircularBufferDouble *cb);

// Returns true if the buffer is empty
bool cb_is_empty(CircularBufferDouble *cb);

// Adds a value to the buffer (returns false if full)
bool cb_push(CircularBufferDouble *cb, double data);

// Removes a value from the buffer (returns false if empty)
bool cb_pop(CircularBufferDouble *cb, double *data);

// Peeks at the next value without removing (returns false if empty)
bool cb_peek(CircularBufferDouble *cb, double *data);

void cb_push_sample(CircularBufferDouble *cb, double data);
void cb_reset(CircularBufferDouble *cb);

#endif // CIRCULAR_BUFFER_H
