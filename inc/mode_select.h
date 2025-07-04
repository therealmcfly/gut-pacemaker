#ifndef MODE_SELECT_H
#define MODE_SELECT_H

typedef enum
{
	MODE_STATIC_DATASET = 1,
	MODE_REALTIME_DATASET,
	MODE_GUT_MODEL,
	MODE_TEST
} RunMode;

RunMode select_mode(void);
int static_dataset_mode(int argc, char *argv[]);
int realtime_dataset_mode(int argc, char *argv[]);
int gut_model_mode(int argc, char *argv[]);
int test_mode(int argc, char *argv[]);

#endif // MODE_SELECT_H