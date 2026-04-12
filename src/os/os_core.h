#ifndef OS_CORE_H
#define OS_CORE_H

#include "../process/processs.h"
#include "../scheduler/scheduler.h"

typedef struct {
	int clock_tick;
	int is_running;
	int current_pid;
	ProcessState current_state;
	int ready_queue_size;
	int blocked_queue_size;
	SchedulerAlgorithm algorithm;
} OSSnapshot;

extern void os_init(SchedulerAlgorithm algorithm);
extern void os_reset(void);

extern void os_start(void);
extern void os_pause(void);
extern int os_is_running(void);

extern int os_step(void);
extern int os_tick(void);

extern void os_set_scheduler_algorithm(SchedulerAlgorithm algorithm);
extern SchedulerAlgorithm os_get_scheduler_algorithm(void);

extern int os_get_clock(void);
extern Process *os_get_running_process(void);
extern int os_is_idle(void);

extern OSSnapshot os_get_snapshot(void);

#endif



