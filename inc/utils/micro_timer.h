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
	struct timespec t_start, t_proc, t_check, t_e2e;
	uint8_t proc_running;
	uint8_t check_running;
	uint8_t e2e_running;
	uint32_t proc_et_us;
	uint32_t check_et_us;
	uint32_t e2e_et_us;
} MicroTimer;

/* tiny hot-path helpers as header inlines */
static inline void mt_init(MicroTimer *mt)
{
	mt->proc_running = 0;
	mt->check_running = 0;
	mt->e2e_running = 0;

	mt->proc_et_us = 0;
	mt->check_et_us = 0;
	mt->e2e_et_us = 0;
}
static inline void mt_start(MicroTimer *mt)
{
	mt->proc_running = 1;
	mt->check_running = 1;
	mt->e2e_running = 1;
	clock_gettime(MT_CLOCK_ID, &mt->t_start);
}

static inline void mt_proc_stop(MicroTimer *mt)
{
	mt->proc_running = 0;
	clock_gettime(MT_CLOCK_ID, &mt->t_proc);
}
static inline void mt_check_stop(MicroTimer *mt)
{
	mt->check_running = 0;
	clock_gettime(MT_CLOCK_ID, &mt->t_check);
}

static inline void mt_e2e_stop(MicroTimer *mt)
{
	mt->e2e_running = 0;
	clock_gettime(MT_CLOCK_ID, &mt->t_e2e);
}

static inline void proc_elapsed_us(MicroTimer *mt)
{
	int64_t proc_ds = (int64_t)mt->t_proc.tv_sec - (int64_t)mt->t_start.tv_sec;
	int64_t proc_dns = (int64_t)mt->t_proc.tv_nsec - (int64_t)mt->t_start.tv_nsec;
	int64_t proc_ns = proc_ds * 1000000000LL + proc_dns;
	if (proc_ns < 0)
		proc_ns = 0;
	mt->proc_et_us = (uint32_t)(proc_ns / 1000LL);
}

static inline void check_elapsed_us(MicroTimer *mt)
{
	int64_t check_ds = (int64_t)mt->t_check.tv_sec - (int64_t)mt->t_start.tv_sec;
	int64_t check_dns = (int64_t)mt->t_check.tv_nsec - (int64_t)mt->t_start.tv_nsec;
	int64_t check_ns = check_ds * 1000000000LL + check_dns;
	if (check_ns < 0)
		check_ns = 0;
	mt->check_et_us = (uint32_t)(check_ns / 1000LL);
}

static inline void e2e_elapsed_us(MicroTimer *mt)
{
	int64_t e2e_ds = (int64_t)mt->t_e2e.tv_sec - (int64_t)mt->t_start.tv_sec;
	int64_t e2e_dns = (int64_t)mt->t_e2e.tv_nsec - (int64_t)mt->t_start.tv_nsec;
	int64_t e2e_ns = e2e_ds * 1000000000LL + e2e_dns;
	if (e2e_ns < 0)
		e2e_ns = 0;
	mt->e2e_et_us = (uint32_t)(e2e_ns / 1000LL);
}

static inline void mt_elapsed_update(MicroTimer *mt)
{
	proc_elapsed_us(mt);
	check_elapsed_us(mt);
	e2e_elapsed_us(mt);
}

/* heavier helper lives in .c */
uint32_t mt_measure_overhead_us(unsigned iters);

void run_measurement(void);

#endif
