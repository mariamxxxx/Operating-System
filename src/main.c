#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter/parser.h"
#include "os/os_core.h"
#include "gui/gui.h"

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

    if (gui_is_initialized()) {
        char *gui_choice = gui_prompt_input("Scheduler: 1=RR, 2=HRRN, 3=MLFQ");
        if (gui_choice != NULL) {
            choice[0] = gui_choice[0];
            choice[1] = '\0';
            free(gui_choice);
        } else {
            choice[0] = '\0';
        }
    } else {
        choice[0] = '\0';
    }

    if (choice[0] == '\0') {
        printf("Choose scheduler (1=RR, 2=HRRN, 3=MLFQ) [default=2]: ");
        if (fgets(choice, sizeof(choice), stdin) == NULL) {
            return HRRN;
        }
    }

    if (choice[0] == '1') {
        gui_log_output("Scheduler: RR");
        return RR;
    }

    if (choice[0] == '3') {
        gui_log_output("Scheduler: MLFQ");
        return MLFQ;
    }

    gui_log_output("Scheduler: HRRN");
    return HRRN;
}

int main(void) {
    printf("main: starting simulator\n");

    gui_init();

    const char *programs[] = {
        "Program_1.txt",
        "Program_2.txt",
        "Program_3.txt"
    };
    const int arrival_times[] = {0, 1, 4};

    while (1) {
        int loaded[] = {0, 0, 0};
        int remaining_to_load = 3;
        int run_steps = 0;
        int safety_cycles = 1000;
        int safety_enabled = 0;

        gui_reset_state();
        SchedulerAlgorithm algorithm = read_algorithm_choice();
        os_init(algorithm);
        gui_note_event("Simulator initialized");
        os_pause();

        while (1) {
            GuiControl control = gui_get_control();
            if (control.action == GUI_ACTION_QUIT) {
                gui_shutdown();
                return 0;
            }

            if (control.action == GUI_ACTION_PAUSE) {
                os_pause();
            } else if (control.action == GUI_ACTION_RUN) {
                run_steps = control.run_steps;
                os_start();
            } else if (control.action == GUI_ACTION_STEP) {
                run_steps = 1;
                os_start();
            }

            if (!os_is_running()) {
                gui_render_idle();
                continue;
            }

            int clock = os_get_clock();

            for (int i = 0; i < 3; i++) {
                if (!loaded[i] && arrival_times[i] == clock) {
                    char path[512];
                    build_program_path(programs[i], path, sizeof(path));
                    printf("main: loading %s at clock=%d\n", path, clock);
                    loadAndInterpret(path, arrival_times[i]);
                    loaded[i] = 1;
                    remaining_to_load--;
                }
            }

            flush_pending_rr_process();

            if (remaining_to_load == 0 && os_is_idle()) {
                gui_note_event("System idle. Session complete.");
                gui_render_idle();
                os_pause();
                break;
            }

            os_tick();
            OSSnapshot snapshot = os_get_snapshot();
            if (snapshot.current_pid == -1) {
                gui_render_idle();
            }

            if (remaining_to_load == 0 &&
                snapshot.current_pid == -1 &&
                snapshot.ready_queue_size == 0 &&
                snapshot.blocked_queue_size > 0) {
                printf("Simulation stopped: deadlock (all remaining processes are blocked).\n");
                gui_note_event("Deadlock detected. Session paused.");
                os_pause();
                break;
            }

            if (run_steps > 0) {
                run_steps--;
                if (run_steps == 0) {
                    os_pause();
                }
            }

            if (safety_enabled) {
                safety_cycles--;
                if (safety_cycles <= 0) {
                    printf("Simulation stopped by safety limit.\n");
                    gui_note_event("Safety limit reached. Session paused.");
                    os_pause();
                    break;
                }
            }
        }
    }
}


// int main(void) {
//     int safety_cycles = 1000;
//     printf("main: starting simulator\n");

//     SchedulerAlgorithm algorithm = read_algorithm_choice();
//     os_init(algorithm);

//     const char *programs[] = {
//         "Program_3.txt"
//     };
//     const int arrival_times[] = {4};
//     int loaded[] = {0};
//     int remaining_to_load = 1;

//     os_start();
//     while (safety_cycles-- > 0) {
//         int clock = os_get_clock();

//         for (int i = 0; i < 1; i++) {
//             if (!loaded[i] && arrival_times[i] == clock) {
//                 char path[512];
//                 build_program_path(programs[i], path, sizeof(path));
//                 printf("main: loading %s at clock=%d\n", path, clock);
//                 loadAndInterpret(path, arrival_times[i]);
//                 loaded[i] = 1;
//                 remaining_to_load--;
//             }
//         }

//         flush_pending_rr_process();

//         if (remaining_to_load == 0 && os_is_idle()) {
//             break;
//         }

//         os_tick();
//         OSSnapshot snapshot = os_get_snapshot();
//         printf("Tick=%d Running=%d Ready=%d Blocked=%d\n",
//                snapshot.clock_tick,
//                snapshot.current_pid,
//                snapshot.ready_queue_size,
//                snapshot.blocked_queue_size);

//         if (remaining_to_load == 0 &&
//             snapshot.current_pid == -1 &&
//             snapshot.ready_queue_size == 0 &&
//             snapshot.blocked_queue_size > 0) {
//             printf("Simulation stopped: deadlock (all remaining processes are blocked).\n");
//             break;
//         }
//     }

//     os_pause();

//     if (safety_cycles <= 0) {
//         printf("Simulation stopped by safety limit.\n");
//     }

//     return 0;
// }