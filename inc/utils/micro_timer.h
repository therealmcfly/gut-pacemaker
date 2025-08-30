#ifndef MICRO_TIMER_H
#define MICRO_TIMER_H

#include <stdint.h>
#include <time.h>

#ifndef MT_CLOCK_ID
#ifdef CLOCK_MONOTONIC_RAW
#define MT_CLOCK_ID CLOCK_MONOTONIC_RAW
#else
#define MT_CLOCK_ID CLOCK_MONOTONIC
#endif
#endif

typedef struct
{
	struct timespec t0, t1;
	uint8_t running;
	uint8_t wc_updated; // set if wcet_us updated on last stop
	uint32_t et_us;			// last elapsed in microseconds
	uint32_t wcet_us;		// worst-case elapsed in microseconds
} MicroTimer;

/* tiny hot-path helpers as header inlines */
static inline void mt_init(MicroTimer *mt)
{
	mt->running = 0;
	mt->wc_updated = 0;
	mt->et_us = 0;
	mt->wcet_us = 0;
}
static inline void mt_start(MicroTimer *mt)
{
	mt->running = 1;
	clock_gettime(MT_CLOCK_ID, &mt->t0);
}
static inline void mt_stop(MicroTimer *mt)
{
	clock_gettime(MT_CLOCK_ID, &mt->t1);
	mt->running = 0;
}
static inline uint32_t mt_elapsed_us(const MicroTimer *mt)
{
	int64_t ds = (int64_t)mt->t1.tv_sec - (int64_t)mt->t0.tv_sec;
	int64_t dns = (int64_t)mt->t1.tv_nsec - (int64_t)mt->t0.tv_nsec;
	int64_t ns = ds * 1000000000LL + dns;
	if (ns < 0)
		ns = 0;
	return (uint32_t)(ns / 1000LL);
}
static inline void mt_update(MicroTimer *mt)
{
	mt->wc_updated = 0;
	mt->et_us = mt_elapsed_us(mt);
	if (mt->et_us > mt->wcet_us)
	{
		mt->wcet_us = mt->et_us;
		mt->wc_updated = 1;
	}
}

/* heavier helper lives in .c */
uint32_t mt_measure_overhead_us(unsigned iters);

void run_measurement(void);

#endif
