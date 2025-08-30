#ifndef MODE_SELECT_H
#define MODE_SELECT_H

typedef enum
{
	MODE_STATIC_DATASET = 1,
	MODE_REALTIME_DATASET,
	MODE_SIL,
	MODE_HIL,
	MODE_HIL_NO_LOG
} RunMode;

RunMode select_mode(void);
int static_dataset_mode(int argc, char *argv[]);
int realtime_dataset_mode(int argc, char *argv[]);
int sil_mode_tcp(int argc, char *argv[]);
int hil_mode_uart(int argc, char *argv[]);

#endif // MODE_SELECT_H