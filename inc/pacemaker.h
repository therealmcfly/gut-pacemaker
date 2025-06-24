#ifndef PACEMAKER_H
#define PACEMAKER_H

#include "shared_data.h"

int run_pacemaker(PacemakerData *p_data, ChannelData *ch_data, int *timer, void (*callback_unlock_mutex)(void));

#endif