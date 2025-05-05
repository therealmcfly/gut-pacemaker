#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdbool.h>

typedef struct
{
	double buffer[BUFFER_SIZE]; // Fixed-size buffer
	double *head;								// Write pointer
	double *tail;								// Read pointer
	double *end;								// Pointer to buffer + BUFFER_SIZE
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

#endif // CIRCULAR_BUFFER_H
