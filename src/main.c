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

int main(void) {
    int safety_cycles = 1000;
    printf("main: starting simulator\n");

    os_init(HRRN);

    char name[256];
    char path[512];

    printf("Enter program filename (e.g., Program_1.txt): ");
    if (fgets(name, sizeof(name), stdin) == NULL) {
        printf("main: no input provided\n");
        return 1;
    }

    size_t len = strlen(name);
    if (len > 0 && name[len - 1] == '\n') {
        name[len - 1] = '\0';
    }

    build_program_path(name, path, sizeof(path));
    printf("main: loading %s\n", path);
    loadAndInterpret(path, 0);

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
