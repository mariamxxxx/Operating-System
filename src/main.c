#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "interpreter/parser.h"
#include "os/os_core.h"

static void build_program_path(const char *name, char *out, size_t out_size) {
    if (strchr(name, '/') || strchr(name, '\\')) {
        snprintf(out, out_size, "%s", name);
        return;
    }
    snprintf(out, out_size, "src/programs/%s", name);
}

struct ThreadData {
    char program[256];
    int arrival_time;
};

void* load_program(void* arg) {
    struct ThreadData* data = (struct ThreadData*)arg;
    char path[512];
    build_program_path(data->program, path, sizeof(path));
    printf("main: loading %s at arrival_time=%d\n", path, data->arrival_time);
    loadAndInterpret(path, data->arrival_time);
    return NULL;
}
//     char choice[32];

//     //printf("Choose scheduler (1=RR, 2=HRRN, 3=MLFQ) [default=2]: ");

//     if (fgets(choice, sizeof(choice), stdin) == NULL) {
//         return HRRN;
//     }

//     if (choice[0] == '1') {
//         return RR;
//     }

//     if (choice[0] == '3') {
//         return MLFQ;
//     }

//     return MLFQ;
// }

int main(void) {
    int safety_cycles = 1000;
    printf("main: starting simulator\n");

    SchedulerAlgorithm algorithm = HRRN;
    os_init(algorithm);

    struct ThreadData data[3] = {
        {"Program_1.txt", 0},
        {"Program_2.txt", 1},
        {"Program_3.txt", 4}
    };

    pthread_t threads[3];

    // Create threads to load programs concurrently
    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, load_program, &data[i]);
    }

    // Wait for all threads to finish loading
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    os_start();
    while (safety_cycles-- > 0) {
        os_tick();
        OSSnapshot snapshot = os_get_snapshot();
        printf("Tick=%d Running=%d Ready=%d Blocked=%d\n",
               snapshot.clock_tick,
               snapshot.current_pid,
               snapshot.ready_queue_size,
               snapshot.blocked_queue_size);

        if (os_is_idle()) {
            printf("Simulation completed: all processes finished.\n");
            break;
        }
    }

    if (safety_cycles <= 0) {
        printf("Simulation stopped by safety limit.\n");
    }

    return 0;
    
}
