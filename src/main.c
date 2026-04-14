#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter/parser.h"
#include "os/os_core.h"

static void build_program_path(const char *name, char *out, size_t out_size) {
    if (strchr(name, '/') || strchr(name, '\\')) {
        snprintf(out, out_size, "%s", name);
        return;
    }
    snprintf(out, out_size, "src/programs/%s", name);
}
    // //A: We're not taking any input
    // printf("Enter program filename (e.g., Program_1.txt): ");
    // if (fgets(name, sizeof(name), stdin) == NULL) {
    //     printf("main: no input provided\n");
    //     return 1;
    // }

    // size_t len = strlen(name);
    // if (len > 0 && name[len - 1] == '\n') {
    //     name[len - 1] = '\0';
    // }

    // build_program_path(name, path, sizeof(path));
    // printf("main: loading %s\n", path);
    // loadAndInterpret(path, 0);


static SchedulerAlgorithm read_algorithm_choice(void) {
    char choice[32];

    printf("Choose scheduler (1=RR, 2=HRRN, 3=MLFQ) [default=2]: ");
    if (fgets(choice, sizeof(choice), stdin) == NULL) {
        return HRRN;
    }

    if (choice[0] == '1') {
        return RR;
    }

    if (choice[0] == '3') {
        return MLFQ;
    }

    return HRRN;
}

int main(void) {
    int safety_cycles = 1000;
    printf("main: starting simulator\n");

    SchedulerAlgorithm algorithm = read_algorithm_choice();
    os_init(algorithm);

    const char *programs[] = {
        "Program_1.txt",
        "Program_2.txt",
        "Program_3.txt"
    };

    for (int i = 0; i < 3; i++) {
        char path[512];
        build_program_path(programs[i], path, sizeof(path));
        printf("main: loading %s\n", path);
        loadAndInterpret(path, i);
    }

    os_start();
    while (!os_is_idle() && safety_cycles-- > 0) {
        os_tick();
        OSSnapshot snapshot = os_get_snapshot();
        printf("Tick=%d Running=%d Ready=%d Blocked=%d\n",
               snapshot.clock_tick,
               snapshot.current_pid,
               snapshot.ready_queue_size,
               snapshot.blocked_queue_size);
       
    }

    os_pause();

    if (safety_cycles <= 0) {
        printf("Simulation stopped by safety limit.\n");
    }

    return 0;
}
