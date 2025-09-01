#ifndef ET_LOG_H
#define ET_LOG_H

#include <stdint.h>
#include <stdio.h>
#include <micro_timer.h>

// #define N_SAMPLES (280u * 100u) // 280 s @ 100 Hz
#define N_SAMPLES (280u * 100u) // 10 s @ 100 Hz

typedef struct
{
	uint32_t proc_et_us[N_SAMPLES];	 // processing-only time
	uint32_t e2e_et_us[N_SAMPLES];	 // end-to-end latency
	uint32_t check_et_us[N_SAMPLES]; // end-to-end latency check time

	uint8_t state[N_SAMPLES];		// pacemaker state per cycle
	uint32_t c_num[N_SAMPLES];	// cycle number at e2e measurement
	uint8_t skipped[N_SAMPLES]; // 1 if buffer skipped (e2e timer not updated)
	uint32_t n;									// current processing cycle number
} EtLog;

static inline void etlog_init(EtLog *L) { L->n = 0; }

int etlog_push(EtLog *L, uint32_t proc_et_us, uint32_t e2e_et_us, uint32_t check_et_us, uint8_t state, uint32_t c_num, uint8_t skipped);

void etlog_dump_csv(const char *path, const EtLog *L);

void et_log_or_dump(uint32_t proc_et_us, uint32_t check_et_us, uint32_t e2e_et_us, EtLog *et_log_ptr, int *et_csv_dumped_ptr, int *et_buffer_full_ptr, uint8_t pm_state, uint32_t c_num, uint8_t skipped);

void save_csv_on_exit(EtLog *et_log_ptr);

#endif // ET_LOG_H
