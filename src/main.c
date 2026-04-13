#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "interpreter/parser.h"
#include "os/os_core.h"

typedef struct {
    const char *filename;
    int arrival_time;
    pthread_mutex_t *load_mutex;
} ProgramLoadArgs;

static void *load_program_thread(void *arg) {
    ProgramLoadArgs *args = (ProgramLoadArgs *) arg;

    pthread_mutex_lock(args->load_mutex);
    printf("main: thread loading %s\n", args->filename);
    loadAndInterpret(args->filename, args->arrival_time);
    pthread_mutex_unlock(args->load_mutex);

    return NULL;
}

int main(void) {
    int safety_cycles = 1000;
    printf("main: starting simulator\n");

    os_init(RR);

    pthread_mutex_t load_mutex;
    pthread_mutex_init(&load_mutex, NULL);

    const char *programs[] = {
        "src/programs/Program_1.txt",
        "src/programs/Program_2.txt",
        "src/programs/Program_3.txt"
    };
    const int arrival_times[] = {1, 2, 3};
    const int program_count = sizeof(programs) / sizeof(programs[0]);

    #define PROGRAM_COUNT 3
    pthread_t threads[PROGRAM_COUNT];
    ProgramLoadArgs args[PROGRAM_COUNT];

    for (int i = 0; i < program_count && i < PROGRAM_COUNT; i++) {
        args[i].filename = programs[i];
        args[i].arrival_time = arrival_times[i];
        args[i].load_mutex = &load_mutex;
        pthread_create(&threads[i], NULL, load_program_thread, &args[i]);
    }

    for (int i = 0; i < program_count; i++) {
        pthread_join(threads[i], NULL);
        printf("main: thread finished loading %s\n", programs[i]);
    }

    pthread_mutex_destroy(&load_mutex);

    os_start();
    while (!os_is_idle() && safety_cycles-- > 0) {
        OSSnapshot snapshot = os_get_snapshot();
        printf("Tick=%d Running=%d Ready=%d Blocked=%d\n",
               snapshot.clock_tick,
               snapshot.current_pid,
               snapshot.ready_queue_size,
               snapshot.blocked_queue_size);
        os_tick();
    }

    os_pause();

    if (safety_cycles <= 0) {
        printf("Simulation stopped by safety limit.\n");
    }

    return 0;
}
