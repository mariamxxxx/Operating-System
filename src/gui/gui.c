#include "gui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

#include "../memory/memoryy.h"
#include "../os/os_core.h"
#include "../scheduler/queue.h"

#define GUI_MIN_WIDTH 1100
#define GUI_MIN_HEIGHT 680

static int g_initialized = 0;
static int g_target_fps = 30;
static char g_last_event[256] = "(none)";
static char g_last_instruction[256] = "(none)";
static int g_last_pid = -1;
static int g_last_pc = -1;
static ProcessState g_last_state = READY;
static GuiTimesliceInfo g_last_slice = {-1, -1};
static int g_last_swap_pid = -1;
static char g_last_swap_action[32] = "";
static char g_swap_lines[48][256];
static int g_swap_line_count = 0;

#define GUI_LOG_LINES 200
static char g_log_lines[GUI_LOG_LINES][256];
static int g_log_index = 0;
static int g_log_scroll = 0;

static int gui_width(void) {
    int w = GetScreenWidth();
    return (w < GUI_MIN_WIDTH) ? GUI_MIN_WIDTH : w;
}

static int gui_height(void) {
    int h = GetScreenHeight();
    return (h < GUI_MIN_HEIGHT) ? GUI_MIN_HEIGHT : h;
}

static const char *state_name(ProcessState state) {
    switch (state) {
        case NEW: return "NEW";
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case BLOCKED: return "BLOCKED";
        case FINISHED: return "FINISHED";
        case SWAPPED: return "SWAPPED";
        default: return "UNKNOWN";
    }
}

static const char *algorithm_name(SchedulerAlgorithm algo) {
    switch (algo) {
        case RR: return "RR";
        case HRRN: return "HRRN";
        case MLFQ: return "MLFQ";
        default: return "UNKNOWN";
    }
}

static void build_queue_string(const Queue *queue, char *out, size_t out_size) {
    if (queue == NULL || queue->head == NULL) {
        snprintf(out, out_size, "(empty)");
        return;
    }

    size_t used = 0;
    QueueNode *current = queue->head;
    while (current != NULL && used + 8 < out_size) {
        int pid = current->process->pcb->pid;
        int written = snprintf(out + used, out_size - used, "P%d", pid);
        if (written < 0) {
            break;
        }
        used += (size_t)written;
        if (current->next != NULL && used + 5 < out_size) {
            written = snprintf(out + used, out_size - used, " -> ");
            if (written < 0) {
                break;
            }
            used += (size_t)written;
        }
        current = current->next;
    }
}

static void draw_button(Rectangle rect, const char *label, int active) {
    Color fill = active ? (Color){36, 120, 110, 255} : (Color){48, 52, 64, 255};
    DrawRectangleRec(rect, fill);
    DrawRectangleLinesEx(rect, 2, (Color){120, 180, 170, 255});
    int font_size = 18;
    int text_width = MeasureText(label, font_size);
    DrawText(label,
             (int)(rect.x + (rect.width - text_width) / 2),
             (int)(rect.y + (rect.height - font_size) / 2),
             font_size,
             RAYWHITE);
}

static void draw_memory_panel(int x, int y, int width, int height) {
    DrawRectangle(x, y, width, height, (Color){20, 24, 32, 255});
    DrawRectangleLines(x, y, width, height, (Color){80, 92, 110, 255});

    int line_height = 16;
    int font_size = 14;
    int max_lines = height / line_height;

    int line = 0;
    for (int i = 0; i < MEMORY_SIZE && line < max_lines; i++) {
        char buffer[256];
        if (mem[i].isFree) {
            snprintf(buffer, sizeof(buffer), "[%02d] FREE", i);
        } else if (mem[i].type == VARIABLE) {
            snprintf(buffer, sizeof(buffer), "[%02d] PID=%d VAR %s=%s",
                     i, mem[i].ownerPid, mem[i].payload.var.name, mem[i].payload.var.value);
        } else if (mem[i].type == CODE_LINE) {
            snprintf(buffer, sizeof(buffer), "[%02d] PID=%d CODE %s",
                     i, mem[i].ownerPid, mem[i].payload.code_line);
        } else {
            snprintf(buffer, sizeof(buffer), "[%02d] PID=%d PCB", i, mem[i].ownerPid);
        }

        DrawText(buffer, x + 10, y + 8 + line * line_height, font_size, RAYWHITE);
        line++;
    }
}

static void draw_swap_panel(int x, int y, int width, int height) {
    DrawRectangle(x, y, width, height, (Color){20, 24, 32, 255});
    DrawRectangleLines(x, y, width, height, (Color){80, 92, 110, 255});

    DrawText("Disk Swap", x + 10, y + 8, 16, (Color){200, 210, 220, 255});

    int line_height = 16;
    int font_size = 14;
    int max_lines = (height - 30) / line_height;
    if (max_lines < 1) {
        max_lines = 1;
    }
    int line = 0;

    if (g_last_swap_pid < 0 || g_swap_line_count == 0) {
        DrawText("(no swap activity)", x + 10, y + 30, font_size, RAYWHITE);
        return;
    }

    for (int i = 0; i < g_swap_line_count && line < max_lines; i++) {
        DrawText(g_swap_lines[i], x + 10, y + 30 + line * line_height, font_size, RAYWHITE);
        line++;
    }
}

static void draw_io_panel(int x, int y, int width, int height) {
    DrawRectangle(x, y, width, height, (Color){20, 24, 32, 255});
    DrawRectangleLines(x, y, width, height, (Color){80, 92, 110, 255});

    DrawText("I/O Log", x + 10, y + 8, 16, (Color){200, 210, 220, 255});

    int line_height = 16;
    int font_size = 14;
    int max_lines = (height - 30) / line_height;
    int line = 0;

    int start = g_log_index - max_lines - g_log_scroll;
    if (start < 0) {
        start = 0;
    }

    for (int i = start; i < g_log_index && line < max_lines; i++) {
        int idx = i % GUI_LOG_LINES;
        DrawText(g_log_lines[idx], x + 10, y + 30 + line * line_height, font_size, RAYWHITE);
        line++;
    }
}

static void render_frame_body(void) {
    ClearBackground((Color){12, 14, 18, 255});

    int width = gui_width();
    int height = gui_height();

    DrawText("OS Simulation Dashboard", 32, 20, 26, (Color){90, 200, 190, 255});

    SchedulerAlgorithm algo = get_current_algo();
    int clock_tick = os_get_clock();
    int is_running = os_is_running();

    char header[256];
    snprintf(header, sizeof(header), "Clock: %d | Mode: %s | Scheduler: %s",
             clock_tick, is_running ? "RUNNING" : "PAUSED", algorithm_name(algo));
    DrawText(header, 32, 58, 18, RAYWHITE);

    if (g_last_slice.time_slice > 0) {
        char slice[128];
        if (algo == MLFQ && g_last_slice.queue_level >= 0) {
            snprintf(slice, sizeof(slice), "Time Slice: %d (MLFQ Q%d)", g_last_slice.time_slice, g_last_slice.queue_level);
        } else {
            snprintf(slice, sizeof(slice), "Time Slice: %d", g_last_slice.time_slice);
        }
        DrawText(slice, 32, 82, 16, (Color){200, 190, 120, 255});
    }

    int panel_x = 32;
    int panel_y = 120;
    int panel_w = 560;
    int panel_h = 220;

    DrawRectangle(panel_x, panel_y, panel_w, panel_h, (Color){20, 24, 32, 255});
    DrawRectangleLines(panel_x, panel_y, panel_w, panel_h, (Color){80, 92, 110, 255});

    char running_line[256];
    if (g_last_pid >= 0) {
        snprintf(running_line, sizeof(running_line), "Running: P%d | State: %s | PC: %d",
                 g_last_pid, state_name(g_last_state), g_last_pc);
    } else {
        snprintf(running_line, sizeof(running_line), "Running: (idle)");
    }
    DrawText(running_line, panel_x + 16, panel_y + 16, 18, RAYWHITE);

    char instruction_line[300];
    snprintf(instruction_line, sizeof(instruction_line), "Instruction: %s", g_last_instruction);
    DrawText(instruction_line, panel_x + 16, panel_y + 44, 16, (Color){200, 220, 240, 255});

    char event_line[300];
    snprintf(event_line, sizeof(event_line), "Last Event: %s", g_last_event);
    DrawText(event_line, panel_x + 16, panel_y + 70, 16, (Color){150, 210, 180, 255});

    char ready_line[512];
    char blocked_line[512];
    build_queue_string(&os_ready_queue, ready_line, sizeof(ready_line));
    build_queue_string(&general_blocked_queue, blocked_line, sizeof(blocked_line));

    DrawText("Ready Queue:", panel_x + 16, panel_y + 106, 16, (Color){230, 210, 120, 255});
    DrawText(ready_line, panel_x + 16, panel_y + 126, 14, RAYWHITE);

    DrawText("Blocked Queue:", panel_x + 16, panel_y + 150, 16, (Color){230, 210, 120, 255});
    DrawText(blocked_line, panel_x + 16, panel_y + 170, 14, RAYWHITE);

    if (algo == MLFQ) {
        int mlfq_y = panel_y + 190;
        for (int i = 0; i < 4; i++) {
            char label[32];
            char queue_line[512];
            snprintf(label, sizeof(label), "MLFQ Q%d:", i);
            build_queue_string(&mlfq_queues[i], queue_line, sizeof(queue_line));
            DrawText(label, panel_x + 16, mlfq_y, 14, (Color){180, 200, 230, 255});
            DrawText(queue_line, panel_x + 90, mlfq_y, 14, RAYWHITE);
            mlfq_y += 16;
        }
    }

    Rectangle step_button = (Rectangle){32, 360, 120, 36};
    Rectangle run_button = (Rectangle){168, 360, 120, 36};
    Rectangle pause_button = (Rectangle){304, 360, 120, 36};
    Rectangle quit_button = (Rectangle){440, 360, 120, 36};

    draw_button(step_button, "Step", 0);
    draw_button(run_button, "Run x10", 0);
    draw_button(pause_button, "Pause", 0);
    draw_button(quit_button, "Quit", 0);

    DrawText("Controls: Space=Step, R=Run 10, P=Pause, Esc=Quit",
             32, 410, 14, (Color){150, 160, 180, 255});

    draw_io_panel(32, 440, 560, height - 460);

    int mem_x = 640;
    int mem_w = (width - mem_x - 40) / 2;
    if (mem_w < 260) {
        mem_w = 260;
    }
    int swap_x = mem_x + mem_w + 20;
    int swap_w = width - swap_x - 40;
    if (swap_w < 260) {
        swap_w = 260;
    }

    draw_memory_panel(mem_x, 120, mem_w, height - 200);
    draw_swap_panel(swap_x, 120, swap_w, height - 200);

}

static void render_frame(void) {
    BeginDrawing();
    render_frame_body();
    EndDrawing();
}

static void render_input_overlay(const char *prompt, const char *buffer) {
    render_frame_body();

    int overlay_w = 520;
    int overlay_h = 180;
    int overlay_x = (gui_width() - overlay_w) / 2;
    int overlay_y = (gui_height() - overlay_h) / 2;

    DrawRectangle(overlay_x - 6, overlay_y - 6, overlay_w + 12, overlay_h + 12, (Color){8, 10, 14, 220});
    DrawRectangle(overlay_x, overlay_y, overlay_w, overlay_h, (Color){30, 36, 46, 255});
    DrawRectangleLines(overlay_x, overlay_y, overlay_w, overlay_h, (Color){120, 160, 170, 255});

    DrawText("Input Required", overlay_x + 16, overlay_y + 16, 20, (Color){220, 230, 240, 255});
    DrawText(prompt, overlay_x + 16, overlay_y + 46, 16, (Color){180, 210, 200, 255});

    DrawRectangle(overlay_x + 16, overlay_y + 86, overlay_w - 32, 36, (Color){18, 20, 26, 255});
    DrawRectangleLines(overlay_x + 16, overlay_y + 86, overlay_w - 32, 36, (Color){90, 120, 140, 255});
    DrawText(buffer, overlay_x + 24, overlay_y + 94, 18, RAYWHITE);

    DrawText("Enter to submit, Esc to cancel", overlay_x + 16, overlay_y + 132, 14, (Color){140, 160, 180, 255});
}

void gui_init(void) {
    if (g_initialized) {
        return;
    }

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(GUI_MIN_WIDTH, GUI_MIN_HEIGHT, "OS Simulation GUI");
    SetTargetFPS(g_target_fps);
    g_initialized = 1;
}

void gui_shutdown(void) {
    if (!g_initialized) {
        return;
    }
    CloseWindow();
    g_initialized = 0;
}

int gui_is_initialized(void) {
    return g_initialized;
}

void gui_reset_state(void) {
    g_last_event[0] = '\0';
    strncpy(g_last_event, "(none)", sizeof(g_last_event) - 1);
    g_last_instruction[0] = '\0';
    strncpy(g_last_instruction, "(none)", sizeof(g_last_instruction) - 1);
    g_last_pid = -1;
    g_last_pc = -1;
    g_last_state = READY;
    g_last_slice.time_slice = -1;
    g_last_slice.queue_level = -1;
    g_last_swap_pid = -1;
    g_last_swap_action[0] = '\0';
    g_swap_line_count = 0;
    for (int i = 0; i < GUI_LOG_LINES; i++) {
        g_log_lines[i][0] = '\0';
    }
    g_log_index = 0;
    g_log_scroll = 0;
}

GuiControl gui_get_control(void) {
    GuiControl control = {GUI_ACTION_NONE, 0};

    if (!g_initialized) {
        return control;
    }

    if (WindowShouldClose()) {
        control.action = GUI_ACTION_QUIT;
        return control;
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        g_log_scroll -= (int)wheel;
        if (g_log_scroll < 0) {
            g_log_scroll = 0;
        }
        int visible_lines = (gui_height() - 460 - 30) / 16;
        if (visible_lines < 1) {
            visible_lines = 1;
        }
        int max_scroll = g_log_index - visible_lines;
        if (max_scroll < 0) {
            max_scroll = 0;
        }
        if (g_log_scroll > max_scroll) {
            g_log_scroll = max_scroll;
        }
    }

    Vector2 mouse = GetMousePosition();
    int clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    Rectangle step_button = (Rectangle){32, 360, 120, 36};
    Rectangle run_button = (Rectangle){168, 360, 120, 36};
    Rectangle pause_button = (Rectangle){304, 360, 120, 36};
    Rectangle quit_button = (Rectangle){440, 360, 120, 36};

    if (clicked && CheckCollisionPointRec(mouse, step_button)) {
        control.action = GUI_ACTION_STEP;
        control.run_steps = 1;
        return control;
    }

    if (clicked && CheckCollisionPointRec(mouse, run_button)) {
        control.action = GUI_ACTION_RUN;
        control.run_steps = 10;
        return control;
    }

    if (clicked && CheckCollisionPointRec(mouse, pause_button)) {
        control.action = GUI_ACTION_PAUSE;
        return control;
    }

    if (clicked && CheckCollisionPointRec(mouse, quit_button)) {
        control.action = GUI_ACTION_QUIT;
        return control;
    }

    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
        control.action = GUI_ACTION_STEP;
        control.run_steps = 1;
        return control;
    }

    if (IsKeyPressed(KEY_R)) {
        control.action = GUI_ACTION_RUN;
        control.run_steps = 10;
        return control;
    }

    if (IsKeyPressed(KEY_P)) {
        control.action = GUI_ACTION_PAUSE;
        return control;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        control.action = GUI_ACTION_QUIT;
        return control;
    }

    return control;
}

void gui_note_event(const char *event) {
    if (event == NULL) {
        return;
    }
    strncpy(g_last_event, event, sizeof(g_last_event) - 1);
    g_last_event[sizeof(g_last_event) - 1] = '\0';
}

static void load_swap_snapshot(int pid) {
    g_swap_line_count = 0;
    if (pid < 0) {
        return;
    }

    char path[256];
    snprintf(path, sizeof(path), "src/disk/pid_%d.swap", pid);
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return;
    }

    int word_count = 0;
    if (fread(&word_count, sizeof(int), 1, file) != 1 || word_count <= 0) {
        fclose(file);
        return;
    }

    snprintf(g_swap_lines[g_swap_line_count++], sizeof(g_swap_lines[0]),
             "PID %d (%s)", pid, g_last_swap_action[0] ? g_last_swap_action : "SWAP");
    snprintf(g_swap_lines[g_swap_line_count++], sizeof(g_swap_lines[0]),
             "words=%d", word_count);

    for (int i = 0; i < word_count && g_swap_line_count < (int)(sizeof(g_swap_lines) / sizeof(g_swap_lines[0])); i++) {
        MemoryWord word;
        if (fread(&word, sizeof(MemoryWord), 1, file) != 1) {
            break;
        }

        if (word.type == PCB_FIELD) {
            snprintf(g_swap_lines[g_swap_line_count++], sizeof(g_swap_lines[0]), "[%02d] PCB", i);
        } else if (word.type == VARIABLE) {
            snprintf(g_swap_lines[g_swap_line_count++], sizeof(g_swap_lines[0]), "[%02d] VAR %s=%s", i, word.payload.var.name, word.payload.var.value);
        } else if (word.type == CODE_LINE) {
            snprintf(g_swap_lines[g_swap_line_count++], sizeof(g_swap_lines[0]), "[%02d] CODE %s", i, word.payload.code_line);
        } else {
            snprintf(g_swap_lines[g_swap_line_count++], sizeof(g_swap_lines[0]), "[%02d] UNKNOWN", i);
        }
    }

    fclose(file);
}

void gui_note_swap(int pid, const char *action) {
    if (pid < 0) {
        g_last_swap_pid = -1;
        g_last_swap_action[0] = '\0';
        g_swap_line_count = 0;
        return;
    }

    g_last_swap_pid = pid;
    if (action == NULL) {
        g_last_swap_action[0] = '\0';
    } else {
        strncpy(g_last_swap_action, action, sizeof(g_last_swap_action) - 1);
        g_last_swap_action[sizeof(g_last_swap_action) - 1] = '\0';
    }
    load_swap_snapshot(pid);
}

void gui_log_output(const char *line) {
    if (line == NULL || line[0] == '\0') {
        return;
    }

    int slot = g_log_index % GUI_LOG_LINES;
    strncpy(g_log_lines[slot], line, sizeof(g_log_lines[slot]) - 1);
    g_log_lines[slot][sizeof(g_log_lines[slot]) - 1] = '\0';
    g_log_index++;
    g_log_scroll = 0;
}

void gui_print_queues(const char *event) {
    gui_note_event(event);
    render_frame();
}

void gui_render_tick(const Process *running, const char *instruction, GuiTimesliceInfo slice) {
    if (running != NULL && running->pcb != NULL) {
        g_last_pid = running->pcb->pid;
        g_last_pc = running->pcb->pc;
        g_last_state = running->pcb->state;
    } else {
        g_last_pid = -1;
        g_last_pc = -1;
        g_last_state = READY;
    }

    if (instruction != NULL) {
        strncpy(g_last_instruction, instruction, sizeof(g_last_instruction) - 1);
        g_last_instruction[sizeof(g_last_instruction) - 1] = '\0';
    } else {
        strncpy(g_last_instruction, "(none)", sizeof(g_last_instruction) - 1);
        g_last_instruction[sizeof(g_last_instruction) - 1] = '\0';
    }

    g_last_slice = slice;
    render_frame();
}

void gui_render_idle(void) {
    g_last_pid = -1;
    g_last_pc = -1;
    g_last_state = READY;
    strncpy(g_last_instruction, "(none)", sizeof(g_last_instruction) - 1);
    g_last_instruction[sizeof(g_last_instruction) - 1] = '\0';
    g_last_slice.time_slice = -1;
    g_last_slice.queue_level = -1;
    render_frame();
}

char *gui_prompt_input(const char *prompt) {
    if (!g_initialized) {
        return NULL;
    }

    char buffer[128] = {0};
    int length = 0;

    while (!WindowShouldClose()) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126 && length < (int)(sizeof(buffer) - 1)) {
                buffer[length++] = (char)key;
                buffer[length] = '\0';
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE) && length > 0) {
            length--;
            buffer[length] = '\0';
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            return strdup("");
        }

        if (IsKeyPressed(KEY_ENTER)) {
            return strdup(buffer);
        }

        BeginDrawing();
        render_input_overlay(prompt != NULL ? prompt : "Enter input:", buffer);
        EndDrawing();
    }

    return strdup("");
}

char *gui_prompt_input_for_pid(const char *prompt, int pid) {
    char formatted[256];
    if (pid >= 0) {
        snprintf(formatted, sizeof(formatted), "P%d: %s", pid, prompt != NULL ? prompt : "Enter input:");
        return gui_prompt_input(formatted);
    }
    return gui_prompt_input(prompt);
}
