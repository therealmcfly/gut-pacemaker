
#include <et_log.h>

int etlog_push(EtLog *L, uint32_t proc_et_us, uint32_t check_et_us, uint32_t e2e_et_us, uint8_t state, uint32_t c_num, uint8_t skipped)
{
	if (L->n < N_SAMPLES)
	{
		L->c_num[L->n] = c_num;

		L->proc_et_us[L->n] = proc_et_us;
		L->check_et_us[L->n] = check_et_us;
		L->e2e_et_us[L->n] = e2e_et_us;

		L->state[L->n] = state;
		L->skipped[L->n] = skipped;
		L->n++;

		return 0; // success
	}
	else
	{
		return 1; // buffer full
	}
}

void etlog_dump_csv(const char *path, const EtLog *L)
{
	FILE *f = fopen(path, "w");
	if (!f)
	{
		perror("fopen");
		return;
	}
	setvbuf(f, NULL, _IOFBF, 1 << 20); // big buffered write
	fputs("idx,c_num,proc_us,check_us,e2e_us,state,skip\n", f);
	for (uint32_t i = 0; i < L->n; i++)
		fprintf(f, "%u,%u,%u,%u,%u,%u,%u\n", i, L->c_num[i], L->proc_et_us[i], L->check_et_us[i], L->e2e_et_us[i], L->state[i], L->skipped[i]);
	fclose(f);
}

void et_log_or_dump(uint32_t proc_et_us, uint32_t check_et_us, uint32_t e2e_et_us, EtLog *et_log_ptr, int *et_csv_dumped_ptr, int *et_buffer_full_ptr, uint8_t pm_state, uint32_t c_num, uint8_t skipped)
{
	if (!*et_buffer_full_ptr)
	{
		*et_buffer_full_ptr = etlog_push(et_log_ptr, proc_et_us, check_et_us, e2e_et_us, pm_state, c_num, skipped);

		// print all values
		// printf("ET log: n=%u, proc=%u us, check=%u us, e2e=%u us, state=%u, c_num=%u, skipped=%u\n", et_log_ptr->n, proc_et_us, check_et_us, e2e_et_us, pm_state, c_num, skipped);
	}
	else
	{
		if (!*et_csv_dumped_ptr)
		{
			char log_filename[64];
			time_t now = time(NULL);
			struct tm *t = localtime(&now);
			// Format: MMDD_HHMM
			snprintf(log_filename, sizeof(log_filename),
							 "et_log/et_log_%02d%02d_%02d%02d.csv",
							 t->tm_mon + 1, t->tm_mday,
							 t->tm_hour, t->tm_min);
			// dump once to tmpfs (RAM)
			etlog_dump_csv(log_filename, et_log_ptr);
			printf("ET log dumped to %s\n", log_filename);
			*et_csv_dumped_ptr = 1; // Set flag to indicate ET log has been dumped
		}
	}
}

void save_csv_on_exit(EtLog *et_log_ptr)
{
	char log_filename[64];
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	// Format: MMDD_HHMM
	snprintf(log_filename, sizeof(log_filename),
					 "et_log/et_log_%02d%02d_%02d%02d.csv",
					 t->tm_mon + 1, t->tm_mday,
					 t->tm_hour, t->tm_min);
	// dump once to tmpfs (RAM)
	etlog_dump_csv(log_filename, et_log_ptr);
	printf("ET log dumped to %s\n", log_filename);
}