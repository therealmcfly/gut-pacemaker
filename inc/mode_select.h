#ifndef MODE_SELECT_H
#define MODE_SELECT_H

typedef enum
{
	MODE_STATIC_DATASET = 1,
	MODE_REALTIME_DATASET,
	MODE_GUT_MODEL
} RunMode;

RunMode select_mode(void);

#endif // MODE_SELECT_H