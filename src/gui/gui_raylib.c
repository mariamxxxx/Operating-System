#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"

#include "../interpreter/parser.h"
#include "../os/os_core.h"
#include "../scheduler/scheduler.h"

typedef struct {
    const char *name;
    int arrival;
    int loaded;
} ProgramSpec;

static void build_program_path(const char *name, char *out, size_t out_size) {
    if (strchr(name, '/') || strchr(name, '\\')) {
        snprintf(out, out_size, "%s", name);
        return;
    }
    snprintf(out, out_size, "src/programs/%s", name);
}

static void load_due_programs(ProgramSpec *programs, int count) {
    int clock = os_get_clock();

    for (int i = 0; i < count; i++) {
        if (!programs[i].loaded && programs[i].arrival == clock) {
            char path[512];
            build_program_path(programs[i].name, path, sizeof(path));
            loadAndInterpret(path, programs[i].arrival);
            programs[i].loaded = 1;
        }
    }

    flush_pending_rr_process();
}

static int draw_button(Rectangle rect, const char *label, Color color) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, rect);

    DrawRectangleRec(rect, hover ? Fade(color, 0.85f) : color);
    DrawRectangleLinesEx(rect, 2.0f, BLACK);
    DrawText(label, (int)rect.x + 10, (int)rect.y + 10, 20, BLACK);

    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

static const char *algo_name(SchedulerAlgorithm algorithm) {
    switch (algorithm) {
        case RR: return "RR";
        case HRRN: return "HRRN";
        case MLFQ: return "MLFQ";
        default: return "Unknown";
    }
}

int main(void) {
    ProgramSpec programs[] = {
        {"Program_1.txt", 0, 0},
        {"Program_2.txt", 1, 0},
        {"Program_3.txt", 4, 0}
    };

    SchedulerAlgorithm selected_algo = HRRN;
    bool initialized = false;
    float tick_accumulator = 0.0f;

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(1100, 700, "OS Simulator GUI (Raylib)");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_ONE)) selected_algo = RR;
        if (IsKeyPressed(KEY_TWO)) selected_algo = HRRN;
        if (IsKeyPressed(KEY_THREE)) selected_algo = MLFQ;

        if (initialized && os_is_running()) {
            tick_accumulator += GetFrameTime();
            if (tick_accumulator >= 0.40f) {
                load_due_programs(programs, 3);
                os_tick();
                tick_accumulator = 0.0f;
            }
        }

        BeginDrawing();
        ClearBackground((Color){241, 244, 248, 255});

        DrawText("OS Simulator GUI (Raylib)", 30, 20, 34, (Color){26, 54, 93, 255});
        DrawText("Choose scheduler: 1=RR  2=HRRN  3=MLFQ", 30, 66, 20, DARKGRAY);

        if (draw_button((Rectangle){30, 110, 170, 46}, "Init / Reset", (Color){149, 225, 211, 255})) {
            os_init(selected_algo);
            for (int i = 0; i < 3; i++) programs[i].loaded = 0;
            initialized = true;
            tick_accumulator = 0.0f;
        }

        if (draw_button((Rectangle){220, 110, 140, 46}, "Start/Pause", (Color){255, 211, 182, 255}) && initialized) {
            if (os_is_running()) os_pause();
            else os_start();
        }

        if (draw_button((Rectangle){380, 110, 110, 46}, "Step", (Color){255, 172, 183, 255}) && initialized) {
            load_due_programs(programs, 3);
            os_step();
        }

        if (draw_button((Rectangle){510, 110, 180, 46}, "Tick x10", (Color){181, 234, 215, 255}) && initialized) {
            for (int i = 0; i < 10; i++) {
                load_due_programs(programs, 3);
                os_start();
                os_tick();
                os_pause();
            }
        }

        OSSnapshot snapshot = {0};
        if (initialized) snapshot = os_get_snapshot();

        DrawRectangle(30, 190, 1040, 460, (Color){255, 255, 255, 255});
        DrawRectangleLines(30, 190, 1040, 460, (Color){200, 210, 220, 255});

        DrawText(TextFormat("Initialized: %s", initialized ? "Yes" : "No"), 60, 230, 28, BLACK);
        DrawText(TextFormat("Scheduler: %s", algo_name(selected_algo)), 60, 280, 28, BLACK);
        DrawText(TextFormat("Clock Tick: %d", snapshot.clock_tick), 60, 330, 28, BLACK);
        DrawText(TextFormat("Running PID: %d", snapshot.current_pid), 60, 380, 28, BLACK);
        DrawText(TextFormat("Ready Queue Size: %d", snapshot.ready_queue_size), 560, 280, 28, BLACK);
        DrawText(TextFormat("Blocked Queue Size: %d", snapshot.blocked_queue_size), 560, 330, 28, BLACK);
        DrawText(TextFormat("OS State: %s", (initialized && os_is_running()) ? "Running" : "Paused"), 560, 380, 28, BLACK);

        DrawText("Tip: Use buttons to step or auto-run. Program arrivals are loaded at clock 0,1,4.", 30, 668, 18, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
