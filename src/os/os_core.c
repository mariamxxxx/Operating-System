#include "os_core.h"

#include <stddef.h>

#include "../synchronization/mutex.h"

// Memory module is currently included through inconsistent headers, so we forward declare.
void init_memory(void);

static int g_clock_tick = 0;
static int g_is_running = 0;
static int g_is_initialized = 0;
static SchedulerAlgorithm g_algorithm = RR;
static Process *g_running_process = NULL;
static int g_last_dispatched_pid = -1;

void os_init(SchedulerAlgorithm algorithm) {
	g_algorithm = algorithm;
	set_current_algo(algorithm);
	g_clock_tick = 0;
	g_is_running = 0;
	g_running_process = NULL;
	g_last_dispatched_pid = -1;

	init_memory();
	init_scheduler();
	init_mutex();

	g_is_initialized = 1;
}

void os_reset(void) {
	if (!g_is_initialized) {
		return;
	}

	g_clock_tick = 0;
	g_is_running = 0;
	g_running_process = NULL;
	g_last_dispatched_pid = -1;

	init_memory();
	init_scheduler();
	init_mutex();
}

void os_start(void) {
	g_is_running = 1;
}

void os_pause(void) {
	g_is_running = 0;
}

int os_is_running(void) {
	return g_is_running;
}

int os_step(void) {
	Process *scheduled = NULL;
	int executed_pid = -1;

	if (!g_is_initialized) {
		return -1;
	}

	scheduled = schedule_next_process(g_algorithm);
	executed_pid = scheduler_get_last_executed_pid();
	g_running_process = scheduled;
	if (executed_pid != -1) {
		g_last_dispatched_pid = executed_pid;
	} else {
		g_last_dispatched_pid = -1;
	}

	// Advance simulated time on every scheduler tick so deferred arrivals
	// still become eligible even when no process is runnable.
	g_clock_tick++;

	if (g_running_process != NULL && g_running_process->pcb != NULL && g_running_process->pcb->state != RUNNING) {
		g_running_process = NULL;
	}

	return (scheduled != NULL) ? 1 : 0;
}

int os_tick(void) {
	if (!g_is_running) {
		return 0;
	}

	return os_step();
}

void os_set_scheduler_algorithm(SchedulerAlgorithm algorithm) {
	g_algorithm = algorithm;
	set_current_algo(algorithm);
}

SchedulerAlgorithm os_get_scheduler_algorithm(void) {
	return g_algorithm;
}

int os_get_clock(void) {
	return g_clock_tick;
}

Process *os_get_running_process(void) {
	return g_running_process;
}

int os_is_idle(void) {
	if (g_running_process != NULL) {
		return 0;
	}

	if (g_algorithm == MLFQ) {
		for (int i = 0; i < 4; i++) {
			if (mlfq_queues[i].size > 0) {
				return 0;
			}
		}
	}

	if (os_ready_queue.size > 0) {
		return 0;
	}

	if (general_blocked_queue.size > 0) {
		return 0;
	}

	return 1;
}

OSSnapshot os_get_snapshot(void) {
	OSSnapshot snapshot;
	int ready_count = os_ready_queue.size;

	if (g_algorithm == MLFQ) {
		ready_count = 0;
		for (int i = 0; i < 4; i++) {
			ready_count += mlfq_queues[i].size;
		}
	}

	snapshot.clock_tick = g_clock_tick;
	snapshot.is_running = g_is_running;
	snapshot.current_pid = (g_running_process != NULL && g_running_process->pcb != NULL) ? g_running_process->pcb->pid : -1;
	if (snapshot.current_pid == -1 && g_last_dispatched_pid != -1) {
		snapshot.current_pid = g_last_dispatched_pid;
	}
	snapshot.current_state = (g_running_process != NULL && g_running_process->pcb != NULL) ? g_running_process->pcb->state : READY;
	snapshot.ready_queue_size = ready_count;
	snapshot.blocked_queue_size = general_blocked_queue.size;
	snapshot.algorithm = g_algorithm;

	return snapshot;
}
