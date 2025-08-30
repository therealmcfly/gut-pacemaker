#ifndef ET_LOG_H
#define ET_LOG_H

#include <stdint.h>
#include <stdio.h>
#include <micro_timer.h>

#define N_SAMPLES (280u * 100u) // 280 s @ 100 Hz

typedef struct
{
	uint32_t et_us[N_SAMPLES]; // execution time per cycle (µs)
	uint8_t state[N_SAMPLES];	 // pacemaker state per cycle
	uint32_t n;								 // current processing cycle number
	uint8_t s[N_SAMPLES];			 // skipcount per cycle
} EtLog;

static inline void etlog_init(EtLog *L) { L->n = 0; }

static inline int etlog_push(EtLog *L, uint32_t et_us, uint8_t state, uint8_t skip)
{
	if (L->n < N_SAMPLES)
	{
		L->et_us[L->n] = et_us;
		L->state[L->n] = state;
		L->s[L->n] = skip;
		L->n++;

		return 0; // success
	}
	else
	{
		return 1; // buffer full
	}
}

static inline void etlog_dump_csv(const char *path, const EtLog *L)
{
	FILE *f = fopen(path, "w");
	if (!f)
	{
		perror("fopen");
		return;
	}
	setvbuf(f, NULL, _IOFBF, 1 << 20); // big buffered write
	fputs("idx,et_us,state,skip\n", f);
	for (uint32_t i = 0; i < L->n; i++)
		fprintf(f, "%u,%u,%u,%u\n", i, L->et_us[i], L->state[i], L->s[i]);
	fclose(f);
}

void et_log_or_dump(MicroTimer *mt_ptr, EtLog *et_log, int timer_overhead, int *et_csv_dumped, int *et_buffer_full, uint8_t pm_state, uint8_t skip);

#endif // ET_LOG_H
