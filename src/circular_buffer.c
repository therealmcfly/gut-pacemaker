#include <stdbool.h>
#include <stddef.h> // for NULL
#include "config.h"
#include <stdio.h>

typedef struct
{
	double buffer[10]; // Fixed-size buffer
	double *head;			 // write pointer
	double *tail;			 // read pointer
	double *end;			 // one-past-the-end pointer
} CircularBufferDouble;

// Initialize buffer
void cb_init(CircularBufferDouble *cb)
{
	cb->head = cb->buffer;
	cb->tail = cb->buffer;
	cb->end = cb->buffer + 10;
	printf("Circular buffer initialized.\n");
}

// Check if full
bool cb_is_full(CircularBufferDouble *cb)
{
	double *next = cb->head + 1;
	if (next == cb->end)
		next = cb->buffer;
	return next == cb->tail;
}

bool full_check(CircularBufferDouble *cb)
{
	return cb->head == cb->tail;
}

// Check if empty
bool cb_is_empty(CircularBufferDouble *cb)
{
	return cb->head == cb->tail;
}

// Push data
bool cb_push(CircularBufferDouble *cb, double data)
{
	// *(cb->head) = data;
	// // if head is at the end, wrap around
	// if (cb->head == cb->end)
	// 	cb->head = cb->buffer;
	// else
	// 	// Otherwise, just advance the head
	// 	cb->head++;

	// If full, advance tail to discard the oldest value
	if (cb_is_full(cb))
	{
		cb->tail++;
		if (cb->tail == cb->end)
			cb->tail = cb->buffer;
	}

	*(cb->head) = data;

	cb->head++;
	if (cb->head == cb->end)
		cb->head = cb->buffer;

	return false;
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

int cb_push_sample(CircularBufferDouble *cb, double data)
{

	*(cb->head) = data;

	printf("\nStored %f at %p\n", *(cb->head), cb->head);

	cb->head++;
	if (cb->head == cb->end)
		cb->head = cb->buffer;

	// printf("Next head: %p\n", cb->head);
	printf("Next head value: %f at %p\n", *(cb->head), cb->head);
	printf("Tail value: %f at %p\n", *(cb->tail), cb->tail);

	if (full_check(cb))
	{
		return 1;
	}

	return 0;
}
