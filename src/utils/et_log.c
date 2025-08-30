
#include <et_log.h>

void et_log_or_dump(MicroTimer *mt_ptr, EtLog *et_log, int timer_overhead, int *et_csv_dumped_ptr, int *et_buffer_full_ptr, uint8_t pm_state, uint8_t skip)
{

	if (!mt_ptr->running)
	{
		printf("Warning: mt_ptr not running in et_log_or_dump\n");
		return; // If the timer is not running, exit the function
	}
	mt_stop(mt_ptr);	 // Stop the timer for interval processing
	mt_update(mt_ptr); // Update the timer for interval processing
	// printf("pm_state = %d last = %u us, wcet = %u us\n", g_shared_data.pacemaker_data_ptr->pm_state, g_shared_data.ch_datas_prt[ch]->mt_ptr->et_us, g_shared_data.ch_datas_prt[ch]->mt_ptr->wcet_us);
	uint32_t et = (mt_ptr->et_us > timer_overhead) ? (mt_ptr->et_us - timer_overhead) : 0;
	if (!*et_buffer_full_ptr)
	{
		*et_buffer_full_ptr = etlog_push(et_log, et, pm_state, skip); // optional; drop if you only want histogram
		printf("etlog_push: et=%u, state=%u, n=%u\n", et, pm_state, et_log->n);
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
			etlog_dump_csv(log_filename, et_log);
			printf("ET log dumped to %s\n", log_filename);
			*et_csv_dumped_ptr = 1; // Set flag to indicate ET log has been dumped
		}
	}
}