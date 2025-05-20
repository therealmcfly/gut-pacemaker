#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h> // for NULL
#include <config.h>

// Circular buffer size

typedef struct
{
	double buffer[BUFFER_SIZE]; // Fixed-size buffer
	double *head;								// Write pointer
	double *tail;								// Read pointer
	double *end;								// Pointer to buffer + BUFFER_SIZE
	bool is_full;								// Flag to indicate if the buffer is full
	bool ready_to_read;					// Flag to indicate if the buffer is ready
	int write_count;						// count of write after last read
} RingBuffer;

// typedef struct
// {
// 	double *read_
// } BufferState;

// Initializes the buffer
void rb_init(RingBuffer *cb);

// Returns true if the buffer is full
bool rb_is_full(RingBuffer *cb);

// Returns true if the buffer is empty
bool rb_is_empty(RingBuffer *cb);

// Adds a value to the buffer (returns false if full)
bool rb_push(RingBuffer *cb, double data);

// Removes a value from the buffer (returns false if empty)
bool rb_pop(RingBuffer *cb, double *data);

// Peeks at the next value without removing (returns false if empty)
bool rb_peek(RingBuffer *cb, double *data);

void rb_push_sample(RingBuffer *cb, double data);
void rb_reset(RingBuffer *cb);
void rb_snapshot(RingBuffer *cb);

#endif // RING_BUFFER_H
