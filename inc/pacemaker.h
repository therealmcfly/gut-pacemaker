#ifndef PACEMAKER_H
#define PACEMAKER_H

#include "shared_data.h"

int run_pacemaker(SharedData *shared_data, void (*callback_unlock_mutex)(void));

#endif