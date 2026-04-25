#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#if defined(_WIN32)
#include <windows.h>
#endif

#include "../interpreter/parser.h"
#include "../os/os_core.h"
#include "../os/syscalls.h"
#include "../scheduler/scheduler.h"

#define MAX_LOG_LINES 10000
#define VISIBLE_LINES 10
#define MAX_INPUT_LEN 64
#define GUI_BASE_WIDTH 1200
#define GUI_BASE_HEIGHT 850
#define GUI_DEFAULT_WIDTH 960
#define GUI_DEFAULT_HEIGHT 680
#define AUTO_TICK_MS 1000

// Memory Box Definitions
#define MAX_MEM_LINES 1000
#define VISIBLE_MEM_LINES 16

// Modern Dark Theme Colors
SDL_Color C_BG      = {30, 30, 46, 255};   
SDL_Color C_PANEL   = {49, 50, 68, 255};   
SDL_Color C_ACCENT  = {137, 180, 250, 255}; 
SDL_Color C_TEXT    = {205, 214, 244, 255}; 
SDL_Color C_GREEN   = {166, 227, 161, 255}; 
SDL_Color C_RED     = {243, 139, 168, 255}; 
SDL_Color C_YELLOW  = {249, 226, 175, 255}; 
SDL_Color C_INACTIVE= {24, 24, 37, 255}; 

extern char input_text[];
static char log_buffer[MAX_LOG_LINES][128];
static int log_count = 0;
static int scroll_offset = 0; 

// Memory Buffers
static char mem_buffer[MAX_MEM_LINES][128];
static int mem_count = 0;
static int mem_scroll_offset = 0;

char input_text[MAX_INPUT_LEN] = "";
bool is_waiting_for_input = false;
bool input_focused = false; 
bool input_submitted = false; 
static char input_var_hint[MAX_INPUT_LEN] = "";
static bool program_1_loaded = false;
static bool program_2_loaded = false;
static bool program_3_loaded = false;

void gui_log(const char *format, ...);
void gui_memory_log(const char *format, ...);
void gui_memory_clear(void);

void gui_memory_clear(void) {
    mem_count = 0;
    mem_scroll_offset = 0;
}

void gui_memory_log(const char *format, ...) {
    if (mem_count >= MAX_MEM_LINES) return;
    va_list args;
    va_start(args, format);
    vsnprintf(mem_buffer[mem_count], 128, format, args);
    va_end(args);
    mem_count++;
}

static bool submit_input_if_ready(void) {
    if (!is_waiting_for_input || strlen(input_text) == 0) return false;
    gui_log("Input Captured: %s", input_text);
    input_submitted = true;
    is_waiting_for_input = false;
    input_var_hint[0] = '\0';
    return true;
}

static void play_tick_sound(void) {
#if defined(_WIN32)
    MessageBeep(MB_OK);
#endif
}

static int sx_i(int base, float sx) { return (int)(base * sx); }
static int sy_i(int base, float sy) { return (int)(base * sy); }

static SDL_Rect scale_rect(SDL_Rect base, float sx, float sy) {
    SDL_Rect out;
    out.x = sx_i(base.x, sx);
    out.y = sy_i(base.y, sy);
    out.w = sx_i(base.w, sx);
    out.h = sy_i(base.h, sy);
    if (out.w < 1) out.w = 1;
    if (out.h < 1) out.h = 1;
    return out;
}

static void capture_input_var_hint_from_log_line(const char *line) {
    if (line == NULL) return;
    const char *prefix = "Exec: Instr '";
    size_t prefix_len = strlen(prefix);
    if (strncmp(line, prefix, prefix_len) != 0) return;
    const char *instr_start = line + prefix_len;
    const char *instr_end = strchr(instr_start, '\'');
    if (instr_end == NULL || instr_end <= instr_start) return;
    char instruction[128];
    size_t len = (size_t)(instr_end - instr_start);
    if (len >= sizeof(instruction)) len = sizeof(instruction) - 1;
    memcpy(instruction, instr_start, len);
    instruction[len] = '\0';
    if (strncmp(instruction, "010*", 4) != 0 || strstr(instruction, "*input") == NULL) return;
    const char *var_start = instruction + 4;
    const char *var_end = strchr(var_start, '*');
    if (var_end == NULL || var_end <= var_start) return;
    size_t var_len = (size_t)(var_end - var_start);
    if (var_len >= sizeof(input_var_hint)) var_len = sizeof(input_var_hint) - 1;
    memcpy(input_var_hint, var_start, var_len);
    input_var_hint[var_len] = '\0';
}

static void load_arrivals_for_current_tick(void) {
    int tick = os_get_clock();
    if (!program_3_loaded && tick >= 4) {
        loadAndInterpret("src/programs/Program_3.txt", 1);
        gui_log(">> Auto-loaded Program_3 at arrival tick 1");
        program_3_loaded = true;
    }
    if (!program_1_loaded && tick >= 0) {
        loadAndInterpret("src/programs/Program_1.txt", 3);
        gui_log(">> Auto-loaded Program_1 at arrival tick 3");
        program_1_loaded = true;
    }
    if (!program_2_loaded && tick >= 1) {
        loadAndInterpret("src/programs/Program_2.txt", 4);
        gui_log(">> Auto-loaded Program_2 at arrival tick 4");
        program_2_loaded = true;
    }
}

static void execute_single_tick(bool is_auto_tick) {
    flush_pending_rr_process();
    os_step();
    load_arrivals_for_current_tick();
    play_tick_sound();
    if (is_auto_tick) gui_log("Tick %d: Auto step executed.", os_get_clock());
    else gui_log("Tick %d: Step executed.", os_get_clock());
}

// void gui_log(const char *format, ...) {
//     if (log_count >= MAX_LOG_LINES) return;
//     va_list args;
//     va_start(args, format);
//     vsnprintf(log_buffer[log_count], 128, format, args);
//     va_end(args);
//     capture_input_var_hint_from_log_line(log_buffer[log_count]);
//     if (strstr(log_buffer[log_count], "goint to take input") != NULL) is_waiting_for_input = true;
//     log_count++;
//     scroll_offset = 0; 
// }

void gui_log(const char *format, ...) {
    // 1. Format the incoming text into a temporary buffer first
    char temp_buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(temp_buffer, 128, format, args);
    va_end(args);

    // --- NEW ROUTING LOGIC ---
    // Check if it's the start of a memory dump
    if (strstr(temp_buffer, "[MEM]") != NULL) {
        strncpy(mem_buffer[mem_count], temp_buffer, 128);
        mem_count++;
        return; // Don't print this header, the box already says "Memory"
    }

    // Check if it's a memory line (e.g., "[0] FREE" or "[4] PID=2")
    // if (temp_buffer[0] == '[' && strchr(temp_buffer, ']')) {
    //     if (mem_count < MAX_MEM_LINES) {
    //         strncpy(mem_buffer[mem_count], temp_buffer, 128);
    //         mem_count++;
    //     }
    //     return; // STOP here so it doesn't go to the left terminal!
    // }
    // -------------------------

    // 2. Normal logs go to the main terminal (Left Box)
    if (log_count >= MAX_LOG_LINES) return;
    strncpy(log_buffer[log_count], temp_buffer, 128);
    
    capture_input_var_hint_from_log_line(log_buffer[log_count]);
    if (strstr(log_buffer[log_count], "goint to take input") != NULL) {
        is_waiting_for_input = true;
    }
    
    log_count++;
    scroll_offset = 0; 
}

static void fill_rect(SDL_Renderer *r, SDL_Rect rect, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(r, &rect);
}

static void stroke_rect(SDL_Renderer *r, SDL_Rect rect, SDL_Color c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDrawRect(r, &rect);
}

static void draw_text(SDL_Renderer *r, TTF_Font *f, int x, int y, SDL_Color c, const char *t) {
    if (!f || !t || t[0] == '\0') return;
    SDL_Surface *s = TTF_RenderUTF8_Blended(f, t, c);
    if (!s) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, s);
    SDL_Rect d = {x, y, s->w, s->h};
    SDL_RenderCopy(r, tex, NULL, &d);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(s);
}

static void draw_button(SDL_Renderer *r, TTF_Font *f, SDL_Rect rect, const char *label, SDL_Color bg, bool is_active) {
    fill_rect(r, rect, bg);
    if (is_active) stroke_rect(r, rect, C_TEXT); 
    draw_text(r, f, rect.x + 15, rect.y + 10, C_BG, label);
}

static void draw_queue_visualized(SDL_Renderer *r, TTF_Font *title_font, TTF_Font *box_font, SDL_Rect rect, const char *title, Queue *queues[], int num_queues, SDL_Color accent) {
    fill_rect(r, rect, C_PANEL);
    fill_rect(r, (SDL_Rect){rect.x, rect.y, 8, rect.h}, accent);
    draw_text(r, title_font, rect.x + 20, rect.y + 10, C_TEXT, title);
    int total_size = 0;
    for (int i = 0; i < num_queues; i++) total_size += queues[i]->size;
    char count_str[16];
    snprintf(count_str, sizeof(count_str), "%d", total_size);
    draw_text(r, title_font, rect.x + rect.w - 30, rect.y + 10, accent, count_str);
    int px = rect.x + 20, py = rect.y + 45;
    for (int i = 0; i < num_queues; i++) {
        if (!queues[i] || !queues[i]->head) continue;
        QueueNode *curr = queues[i]->head;
        while (curr) {
            if (px + 50 > rect.x + rect.w - 10) { px = rect.x + 20; py += 55; }
            if (py + 45 > rect.y + rect.h) break; 
            SDL_Rect p_box = {px, py, 45, 45};
            fill_rect(r, p_box, C_BG);
            stroke_rect(r, p_box, accent);
            char p_lbl[8];
            snprintf(p_lbl, sizeof(p_lbl), "P%d", curr->process->pcb->pid);
            draw_text(r, box_font, px + 10, py + 12, C_TEXT, p_lbl);
            px += 55; curr = curr->next;
        }
    }
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    SchedulerAlgorithm selected_algo = HRRN;
    bool initialized = false, running = true, auto_run = false, resume_auto_run_after_input = false;
    Uint32 last_auto_tick_ms = 0;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *win = SDL_CreateWindow("OStor OS Pro", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GUI_DEFAULT_WIDTH, GUI_DEFAULT_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetWindowMinimumSize(win, 900, 620);

    TTF_Font *f_sml = TTF_OpenFont("assets/font.ttf", 14);
    TTF_Font *f_reg = TTF_OpenFont("assets/font.ttf", 18);
    TTF_Font *f_bold = TTF_OpenFont("assets/font.ttf", 24);

    if (!f_reg) { printf("CRITICAL: font.ttf missing.\n"); return 1; }

    SDL_Rect btn_rr_base = {50, 80, 100, 40}, btn_hr_base = {160, 80, 100, 40}, btn_ml_base = {270, 80, 100, 40};
    SDL_Rect btn_run_base = {940, 80, 100, 40}, btn_pause_base = {1050, 80, 100, 40}, btn_step_base = {830, 80, 100, 40};
    SDL_Rect input_bar_base = {50, 770, 1100, 50}, btn_enter_base = {1060, 770, 90, 50};
    SDL_Rect ready_rect_base = {50, 140, 530, 140}, blocked_rect_base = {620, 140, 530, 140};
    
    // Adjusted layouts to make room for the new memory box
    SDL_Rect kernel_rect_base = {50, 290, 540, 120};
    SDL_Rect term_rect_base = {50, 430, 540, 320};
    SDL_Rect mem_rect_base = {610, 290, 540, 460};

    gui_log(">> System Online. Click an algorithm to begin.");
    SDL_StopTextInput(); 

    while (running) {
        int win_w, win_h;
        SDL_GetWindowSize(win, &win_w, &win_h);
        float sx = (float)win_w / (float)GUI_BASE_WIDTH, sy = (float)win_h / (float)GUI_BASE_HEIGHT;

        SDL_Rect btn_rr = scale_rect(btn_rr_base, sx, sy), btn_hr = scale_rect(btn_hr_base, sx, sy), btn_ml = scale_rect(btn_ml_base, sx, sy);
        SDL_Rect btn_run = scale_rect(btn_run_base, sx, sy), btn_pause = scale_rect(btn_pause_base, sx, sy), btn_step = scale_rect(btn_step_base, sx, sy);
        SDL_Rect input_bar = scale_rect(input_bar_base, sx, sy), btn_enter = scale_rect(btn_enter_base, sx, sy);
        
        SDL_Rect term_rect = scale_rect(term_rect_base, sx, sy);
        SDL_Rect ready_rect = scale_rect(ready_rect_base, sx, sy), blocked_rect = scale_rect(blocked_rect_base, sx, sy), kernel_rect = scale_rect(kernel_rect_base, sx, sy);
        SDL_Rect mem_rect = scale_rect(mem_rect_base, sx, sy);

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_MOUSEWHEEL) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                // Check if mouse is hovering over memory area or term area for isolated scrolling
                if (mx >= mem_rect.x && my >= mem_rect.y && my <= mem_rect.y + mem_rect.h) {
                    if (ev.wheel.y > 0) { if (mem_scroll_offset > 0) mem_scroll_offset--; }
                    else if (ev.wheel.y < 0) { if (mem_scroll_offset + VISIBLE_MEM_LINES < mem_count) mem_scroll_offset++; }
                } else {
                    if (ev.wheel.y > 0) { if (log_count - VISIBLE_LINES - scroll_offset > 0) scroll_offset++; }
                    else if (ev.wheel.y < 0) { if (scroll_offset > 0) scroll_offset--; }
                }
            }
            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                int mx = ev.button.x, my = ev.button.y;
                if (is_waiting_for_input && mx >= input_bar.x && mx <= input_bar.x + input_bar.w && my >= input_bar.y && my <= input_bar.y + input_bar.h) {
                    input_focused = true; SDL_StartTextInput();
                } else { input_focused = false; SDL_StopTextInput(); }
                if (is_waiting_for_input && mx >= btn_enter.x && mx <= btn_enter.x + btn_enter.w && my >= btn_enter.y && my <= btn_enter.y + btn_enter.h) {
                    if (submit_input_if_ready() && resume_auto_run_after_input) { auto_run = true; resume_auto_run_after_input = false; last_auto_tick_ms = SDL_GetTicks(); }
                }
                if (mx >= btn_rr.x && mx <= btn_rr.x + btn_rr.w && my >= btn_rr.y && my <= btn_rr.y + btn_rr.h) { 
                    selected_algo = RR; os_init(RR); initialized = true; gui_log(">> Init: Round Robin");
                    program_1_loaded = program_2_loaded = program_3_loaded = false; auto_run = false; load_arrivals_for_current_tick();
                }
                if (mx >= btn_hr.x && mx <= btn_hr.x + btn_hr.w && my >= btn_hr.y && my <= btn_hr.y + btn_hr.h) { 
                    selected_algo = HRRN; os_init(HRRN); initialized = true; gui_log(">> Init: HRRN");
                    program_1_loaded = program_2_loaded = program_3_loaded = false; auto_run = false; load_arrivals_for_current_tick();
                }
                if (mx >= btn_ml.x && mx <= btn_ml.x + btn_ml.w && my >= btn_ml.y && my <= btn_ml.y + btn_ml.h) { 
                    selected_algo = MLFQ; os_init(MLFQ); initialized = true; gui_log(">> Init: MLFQ");
                    program_1_loaded = program_2_loaded = program_3_loaded = false; auto_run = false; load_arrivals_for_current_tick();
                }
                if (mx >= btn_run.x && mx <= btn_run.x + btn_run.w && my >= btn_run.y && my <= btn_run.y + btn_run.h && initialized) {
                    auto_run = true; resume_auto_run_after_input = false; last_auto_tick_ms = SDL_GetTicks(); gui_log("Tick auto-run enabled.");
                }
                if (mx >= btn_pause.x && mx <= btn_pause.x + btn_pause.w && my >= btn_pause.y && my <= btn_pause.y + btn_pause.h && initialized) {
                    auto_run = false; resume_auto_run_after_input = false; gui_log("Tick auto-run paused.");
                }
                if (mx >= btn_step.x && mx <= btn_step.x + btn_step.w && my >= btn_step.y && my <= btn_step.y + btn_step.h && initialized) {
                    auto_run = false; resume_auto_run_after_input = false; execute_single_tick(false);
                }
            }
            if (input_focused) {
                if (ev.type == SDL_TEXTINPUT) { if (strlen(input_text) + strlen(ev.text.text) < MAX_INPUT_LEN - 1) strcat(input_text, ev.text.text); }
                if (ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.sym == SDLK_BACKSPACE && strlen(input_text) > 0) input_text[strlen(input_text)-1] = '\0';
                    if (ev.key.keysym.sym == SDLK_RETURN && strlen(input_text) > 0) {
                        if (is_waiting_for_input) {
                            if (submit_input_if_ready() && resume_auto_run_after_input) { auto_run = true; resume_auto_run_after_input = false; last_auto_tick_ms = SDL_GetTicks(); }
                        } else {
                            gui_log("LOAD_CMD: %s", input_text);
                            char path[512]; snprintf(path, sizeof(path), "src/programs/%s", input_text);
                            loadAndInterpret(path, os_get_clock()); input_text[0] = '\0'; 
                        }
                    }
                }
            }
        }

        if (is_waiting_for_input) {
            input_focused = true; SDL_StartTextInput();
            if (auto_run) { auto_run = false; resume_auto_run_after_input = true; gui_log("Paused for input."); }
        }

        fill_rect(ren, (SDL_Rect){0, 0, win_w, win_h}, C_BG);
        draw_text(ren, f_bold, sx_i(50, sx), sy_i(30, sy), C_ACCENT, "OStor OS Dashboard v3.7");
        draw_button(ren, f_reg, btn_rr, "RR", C_ACCENT, selected_algo == RR);
        draw_button(ren, f_reg, btn_hr, "HRRN", C_ACCENT, selected_algo == HRRN);
        draw_button(ren, f_reg, btn_ml, "MLFQ", C_ACCENT, selected_algo == MLFQ);
        draw_button(ren, f_reg, btn_step, "Step", C_GREEN, false);
        draw_button(ren, f_reg, btn_run, "Run", C_ACCENT, auto_run);
        draw_button(ren, f_reg, btn_pause, "Pause", C_RED, !auto_run);

        Queue *ready_qs[4]; int num_ready = 1;
        if (selected_algo == MLFQ) { ready_qs[0] = &mlfq_queues[0]; ready_qs[1] = &mlfq_queues[1]; ready_qs[2] = &mlfq_queues[2]; ready_qs[3] = &mlfq_queues[3]; num_ready = 4; }
        else { ready_qs[0] = &os_ready_queue; }
        draw_queue_visualized(ren, f_reg, f_sml, ready_rect, "Ready Queue", ready_qs, num_ready, C_GREEN);
        Queue *blocked_qs[1] = {&general_blocked_queue};
        draw_queue_visualized(ren, f_reg, f_sml, blocked_rect, "Blocked Queue", blocked_qs, 1, C_RED);

        fill_rect(ren, kernel_rect, C_PANEL); stroke_rect(ren, kernel_rect, C_ACCENT);
        draw_text(ren, f_bold, sx_i(70, sx), sy_i(310, sy), C_ACCENT, "Kernel Execution");
        OSSnapshot snap = os_get_snapshot(); char k_stat[128];
        if (snap.current_pid != -1) snprintf(k_stat, sizeof(k_stat), "Running PID: %d | System Clock: %d", snap.current_pid, snap.clock_tick);
        else snprintf(k_stat, sizeof(k_stat), "CPU Idle | System Clock: %d", snap.clock_tick);
        draw_text(ren, f_reg, sx_i(70, sx), sy_i(350, sy), (snap.current_pid != -1 ? C_GREEN : C_TEXT), k_stat);

        fill_rect(ren, term_rect, (SDL_Color){17, 17, 27, 255}); 
        stroke_rect(ren, term_rect, C_TEXT);
        int s_idx = log_count - VISIBLE_LINES - scroll_offset;
        if (s_idx < 0) s_idx = 0;
        for (int i = 0; i < VISIBLE_LINES && (s_idx + i) < log_count; i++) draw_text(ren, f_sml, sx_i(70, sx), sy_i(445 + (i * 28), sy), C_TEXT, log_buffer[s_idx + i]);
        if (scroll_offset > 0) draw_text(ren, f_sml, term_rect.x + term_rect.w - 120, term_rect.y + 10, C_YELLOW, "[SCROLLED UP]");

        // ================= NEW MEMORY BOX =================
        fill_rect(ren, mem_rect, (SDL_Color){17, 17, 27, 255}); 
        stroke_rect(ren, mem_rect, C_ACCENT);
        draw_text(ren, f_bold, sx_i(630, sx), sy_i(310, sy), C_ACCENT, "Memory");
        
        for (int i = 0; i < VISIBLE_MEM_LINES && (mem_scroll_offset + i) < mem_count; i++) {
            draw_text(ren, f_sml, sx_i(630, sx), sy_i(350 + (i * 24), sy), C_TEXT, mem_buffer[mem_scroll_offset + i]);
        }
        // ==================================================

        bool blink = ((SDL_GetTicks() / 450) % 2) == 0;
        if (is_waiting_for_input) {
            fill_rect(ren, input_bar, (input_focused ? C_PANEL : C_INACTIVE)); stroke_rect(ren, input_bar, (blink ? C_RED : C_ACCENT));
            draw_button(ren, f_reg, btn_enter, "Enter", (strlen(input_text) > 0 ? C_GREEN : C_INACTIVE), (strlen(input_text) > 0));
            if (blink) draw_text(ren, f_reg, sx_i(70, sx), sy_i(785, sy), C_RED, "Awaiting Input >");
            if (strlen(input_text) == 0 && strlen(input_var_hint) > 0) { char hint[128]; snprintf(hint, sizeof(hint), "value for %s", input_var_hint); draw_text(ren, f_reg, sx_i(260, sx), sy_i(785, sy), C_INACTIVE, hint); }
            else draw_text(ren, f_reg, sx_i(260, sx), sy_i(785, sy), C_TEXT, input_text);
        }

        if (initialized && auto_run) {
            Uint32 now = SDL_GetTicks();
            if (now - last_auto_tick_ms >= AUTO_TICK_MS) { execute_single_tick(true); last_auto_tick_ms = now; }
        }
        SDL_RenderPresent(ren);
    }
    TTF_CloseFont(f_sml); TTF_CloseFont(f_reg); TTF_CloseFont(f_bold);
    SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit(); return 0;
}