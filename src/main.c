#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "interpreter/parser.h"
#include "os/os_core.h"
#include "os/syscalls.h"
#include "scheduler/scheduler.h"

// --- GUI TERMINAL SETTINGS ---
#define MAX_LOG_LINES 10
#define MAX_INPUT_LEN 64
static char log_buffer[MAX_LOG_LINES][128];
static int log_count = 0;
static char input_text[MAX_INPUT_LEN] = "";

// Custom logging function for the GUI
void gui_log(const char *format, ...) {
    if (log_count >= MAX_LOG_LINES) {
        for (int i = 0; i < MAX_LOG_LINES - 1; i++) {
            strcpy(log_buffer[i], log_buffer[i + 1]);
        }
        log_count = MAX_LOG_LINES - 1;
    }
    va_list args;
    va_start(args, format);
    vsnprintf(log_buffer[log_count], 128, format, args);
    va_end(args);
    log_count++;
}

typedef struct {
    const char *name;
    int arrival;
    int loaded;
} ProgramSpec;

typedef struct {
    SDL_Rect rect;
    const char *label;
} UIButton;

typedef struct {
    int active;
    int pid;
    int start_x, start_y, end_x, end_y;
    Uint32 start_ms, end_ms;
} DispatchAnimation;

// --- HELPER FUNCTIONS ---
static int point_in_rect(int x, int y, SDL_Rect rect) {
    return x >= rect.x && x < (rect.x + rect.w) && y >= rect.y && y < (rect.y + rect.h);
}

static void fill_rect(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

static void stroke_rect(SDL_Renderer *renderer, SDL_Rect rect, SDL_Color color) {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawRect(renderer, &rect);
}

static void draw_text(SDL_Renderer *renderer, TTF_Font *font, int x, int y, SDL_Color color, const char *text) {
    if (!font || !text || text[0] == '\0') return;
    SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

static void build_program_path(const char *name, char *out, size_t out_size) {
    snprintf(out, out_size, "src/programs/%s", name);
}

static void perform_single_tick(ProgramSpec *programs, int count, SchedulerAlgorithm algo, DispatchAnimation *anim, SDL_Rect queue_lanes[], SDL_Rect kernel_rect, int *last_pid) {
    int clock = os_get_clock();
    
    // Auto-load programs based on arrival time
    for (int i = 0; i < count; i++) {
        if (!programs[i].loaded && programs[i].arrival == clock) {
            char path[512];
            build_program_path(programs[i].name, path, sizeof(path));
            loadAndInterpret(path, programs[i].arrival);
            programs[i].loaded = 1;
            gui_log("System: Loaded %s", programs[i].name);
        }
    }

    flush_pending_rr_process();
    
    if (os_is_idle() && clock > 0) {
        gui_log("System: Idle / Simulation Ended");
        return;
    }

    os_step();
    OSSnapshot snap = os_get_snapshot();
    gui_log("Tick %d: PID %d running. Ready: %d", snap.clock_tick, snap.current_pid, snap.ready_queue_size);

    // Animation trigger
    if (snap.current_pid != -1 && snap.current_pid != *last_pid) {
        anim->active = 1;
        anim->pid = snap.current_pid;
        anim->start_x = queue_lanes[0].x + 50;
        anim->start_y = queue_lanes[0].y;
        anim->end_x = kernel_rect.x + kernel_rect.w / 2;
        anim->end_y = kernel_rect.y + 40;
        anim->start_ms = SDL_GetTicks();
        anim->end_ms = anim->start_ms + 600;
        *last_pid = snap.current_pid;
    }
}

// Reuse your drawing functions (draw_button, draw_value_card, etc. from your snippet)
// [I am omitting them here for brevity but keep them in your file]

int old_main(int argc, char **argv) {
    ProgramSpec programs[] = { {"Program_1.txt", 0, 0}, {"Program_2.txt", 1, 0}, {"Program_3.txt", 4, 0} };
    SchedulerAlgorithm selected_algo = HRRN;
    bool initialized = false, running = true;
    int last_pid = -1;
    DispatchAnimation dispatch_anim = {0};

    if (SDL_Init(SDL_INIT_VIDEO) != 0 || TTF_Init() != 0) return 1;
    SDL_Window *win = SDL_CreateWindow("OS Simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1180, 800, SDL_WINDOW_SHOWN);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    // MAC FONT PATH - Change if needed
    TTF_Font *font = TTF_OpenFont("/Library/Fonts/Arial.ttf", 18);
    TTF_Font *f_large = TTF_OpenFont("/Library/Fonts/Arial.ttf", 24);

    SDL_StartTextInput(); // Enable GUI Input

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            
            // --- INPUT BOX LOGIC ---
            if (ev.type == SDL_TEXTINPUT) {
                strncat(input_text, ev.text.text, MAX_INPUT_LEN - strlen(input_text) - 1);
            }
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_BACKSPACE && strlen(input_text) > 0) 
                    input_text[strlen(input_text) - 1] = '\0';
                if (ev.key.keysym.sym == SDLK_RETURN) {
                    gui_log("User Input: %s", input_text);
                    // Add logic here to handle user commands if needed
                    input_text[0] = '\0';
                }
                if (ev.key.keysym.sym == SDLK_n && initialized) {
                     perform_single_tick(programs, 3, selected_algo, &dispatch_anim, (SDL_Rect[]){{70,250,500,72}}, (SDL_Rect){50,475,1100,170}, &last_pid);
                }
            }
            
            // --- BUTTON LOGIC ---
            if (ev.type == SDL_MOUSEBUTTONDOWN) {
                if (ev.button.x < 150 && ev.button.y < 150) { // Simple Init Button Area
                    os_init(selected_algo);
                    initialized = true;
                    gui_log("System: OS Initialized (%d)", selected_algo);
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 15, 20, 30, 255);
        SDL_RenderClear(ren);

        // --- DRAW TERMINAL LOG ---
        SDL_Rect term_box = {50, 660, 1080, 120};
        fill_rect(ren, term_box, (SDL_Color){5, 5, 10, 255});
        stroke_rect(ren, term_box, (SDL_Color){0, 255, 100, 255});
        
        for (int i = 0; i < log_count; i++) {
            draw_text(ren, font, 60, 665 + (i * 18), (SDL_Color){0, 200, 0, 255}, log_buffer[i]);
        }

        // --- DRAW INPUT FIELD ---
        draw_text(ren, font, 850, 750, (SDL_Color){255, 255, 0, 255}, "CMD > ");
        draw_text(ren, font, 910, 750, (SDL_Color){255, 255, 255, 255}, input_text);

        // --- DRAW KERNEL & STATE ---
        draw_text(ren, f_large, 50, 20, (SDL_Color){100, 200, 255, 255}, "OS Visualizer");
        
        if (dispatch_anim.active) {
            // ... (Insert your animation drawing code here) ...
            dispatch_anim.active = (SDL_GetTicks() < dispatch_anim.end_ms);
        }

        SDL_RenderPresent(ren);
    }

    SDL_StopTextInput();
    TTF_CloseFont(font);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    TTF_Quit();
    SDL_Quit();
    return 0;
}