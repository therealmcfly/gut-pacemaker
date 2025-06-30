#ifndef MULTITHREADING_H
#define MULTITHREADING_H

void *rd_mode_receive_thread(void *data);
void *gut_model_mode_receive_thread(void *ch_ptr);
void *process_thread(void *data);
void *pacemaker_thread(void *ch_ptr);

#endif // MULTITHREADING_H