#include <stdio.h>

#include "interpreter/parser.h"
#include "os/os_core.h"

int main(void) {
    int safety_cycles = 1000;

    os_init(RR);

    // Load at least one program before stepping the scheduler.
    loadAndInterpret("src/programs/Program_1.txt",1);

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
            printf("parseInstructionsIntoMemory: %d, %s");


    return 0;
}
