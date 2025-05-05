#include <stdbool.h>
#include <stddef.h> // for NULL
#include "config.h"

typedef struct
{
	double buffer[BUFFER_SIZE];
	double *head; // write pointer
	double *tail; // read pointer
	double *end;	// one-past-the-end pointer
} CircularBufferDouble;

// Initialize buffer
void cb_init(CircularBufferDouble *cb)
{
	cb->head = cb->buffer;
	cb->tail = cb->buffer;
	cb->end = cb->buffer + BUFFER_SIZE;
}

// Check if full
bool cb_is_full(CircularBufferDouble *cb)
{
	double *next = cb->head + 1;
	if (next == cb->end)
		next = cb->buffer;
	return next == cb->tail;
}

// Check if empty
bool cb_is_empty(CircularBufferDouble *cb)
{
	return cb->head == cb->tail;
}

// Push data
bool cb_push(CircularBufferDouble *cb, double data)
{
	if (cb_is_full(cb))
		return false;

	*(cb->head) = data;
	cb->head++;
	if (cb->head == cb->end)
		cb->head = cb->buffer;
	return true;
}

// Pop data
bool cb_pop(CircularBufferDouble *cb, double *data)
{
	if (cb_is_empty(cb))
		return false;

	*data = *(cb->tail);
	cb->tail++;
	if (cb->tail == cb->end)
		cb->tail = cb->buffer;
	return true;
}

// Peek data
bool cb_peek(CircularBufferDouble *cb, double *data)
{
	if (cb_is_empty(cb))
		return false;

	*data = *(cb->tail);
	return true;
}
