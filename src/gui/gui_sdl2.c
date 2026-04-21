#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h> 
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../interpreter/parser.h"
#include "../os/os_core.h"
#include "../os/syscalls.h"
#include "../scheduler/scheduler.h"

#define MAX_LOG_LINES 1000
#define VISIBLE_LINES 10
#define MAX_INPUT_LEN 64

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
static int log_scroll_offset = 0; 

char input_text[MAX_INPUT_LEN] = "";
bool is_waiting_for_input = false;
bool input_focused = false; 
bool input_submitted = false; // NEW FLAG: Tells the OS the text is ready
char alert_reason[128] = "";

void gui_log(const char *format, ...) {
    if (log_count >= MAX_LOG_LINES) return;
    va_list args;
    va_start(args, format);
    vsnprintf(log_buffer[log_count], 128, format, args);
    va_end(args);
    
    if (strstr(log_buffer[log_count], "goint to take input") != NULL || 
        strstr(log_buffer[log_count], "Enter input:") != NULL) {
        
        is_waiting_for_input = true;
        snprintf(alert_reason, sizeof(alert_reason), "%s", log_buffer[log_count]);
    }
    
    log_count++;
    log_scroll_offset = 0; 
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

    int px = rect.x + 20;
    int py = rect.y + 45;

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

            px += 55;
            curr = curr->next;
        }
    }
}

int main(int argc, char **argv) {

    
    SchedulerAlgorithm selected_algo = HRRN;
    bool initialized = false, running = true;

    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    SDL_Window *win = SDL_CreateWindow("OStor OS Pro", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 850, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font *f_sml = TTF_OpenFont("assets/font.ttf", 14);
    TTF_Font *f_reg = TTF_OpenFont("assets/font.ttf", 18);
    TTF_Font *f_bold = TTF_OpenFont("assets/font.ttf", 24);

    if (!f_reg) { printf("CRITICAL: font.ttf missing from assets folder.\n"); return 1; }

    SDL_Rect btn_rr = {50, 80, 100, 40};
    SDL_Rect btn_hr = {160, 80, 100, 40};
    SDL_Rect btn_ml = {270, 80, 100, 40};
    SDL_Rect btn_step = {1050, 80, 100, 40};
    SDL_Rect input_bar = {50, 770, 1100, 50}; 
    SDL_Rect term_rect = {50, 430, 1100, 320}; 

    gui_log(">> System Online. Click an algorithm (RR, HRRN, MLFQ) to begin.");
    SDL_StopTextInput(); 

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            
            if (ev.type == SDL_MOUSEWHEEL) {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                
                if (mx >= term_rect.x && mx <= term_rect.x + term_rect.w && 
                    my >= term_rect.y && my <= term_rect.y + term_rect.h) {
                    
                    int max_scroll = (log_count > VISIBLE_LINES) ? log_count - VISIBLE_LINES : 0;
                    log_scroll_offset -= ev.wheel.y; 
                    
                    if (log_scroll_offset < 0) log_scroll_offset = 0;
                    if (log_scroll_offset > max_scroll) log_scroll_offset = max_scroll;
                }
            }

            if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                int mx = ev.button.x, my = ev.button.y;
                
                if (mx >= input_bar.x && mx <= input_bar.x + input_bar.w && my >= input_bar.y && my <= input_bar.y + input_bar.h) {
                    input_focused = true;
                    SDL_StartTextInput();
                } else {
                    input_focused = false;
                    SDL_StopTextInput();
                }

                if (mx >= btn_rr.x && mx <= btn_rr.x + btn_rr.w && my >= btn_rr.y && my <= btn_rr.y + btn_rr.h) { 
                    selected_algo = RR; os_init(RR); initialized = true; gui_log(">> Init: Round Robin"); 
                }
                if (mx >= btn_hr.x && mx <= btn_hr.x + btn_hr.w && my >= btn_hr.y && my <= btn_hr.y + btn_hr.h) { 
                    selected_algo = HRRN; os_init(HRRN); initialized = true; gui_log(">> Init: HRRN"); 
                }
                if (mx >= btn_ml.x && mx <= btn_ml.x + btn_ml.w && my >= btn_ml.y && my <= btn_ml.y + btn_ml.h) { 
                    selected_algo = MLFQ; os_init(MLFQ); initialized = true; gui_log(">> Init: MLFQ"); 
                }
                
                if (mx >= btn_step.x && mx <= btn_step.x + btn_step.w && my >= btn_step.y && my <= btn_step.y + btn_step.h && initialized) {
                    os_step();
                    gui_log("Tick %d: Step executed.", os_get_clock());
                }
            }
            
            if (input_focused) {
                if (ev.type == SDL_TEXTINPUT) {
                    if (strlen(input_text) + strlen(ev.text.text) < MAX_INPUT_LEN - 1) {
                        strcat(input_text, ev.text.text);
                    }
                }
                if (ev.type == SDL_KEYDOWN) {
                    if (ev.key.keysym.sym == SDLK_BACKSPACE && strlen(input_text) > 0) {
                        input_text[strlen(input_text)-1] = '\0';
                    }
                    if (ev.key.keysym.sym == SDLK_RETURN && strlen(input_text) > 0) {
                        if (is_waiting_for_input) {
                            gui_log("Input Captured: %s", input_text);
                            input_submitted = true; // Raise the flag for the OS!
                            is_waiting_for_input = false; // Hide the alert box
                        } else {
                            gui_log("LOAD_CMD: %s", input_text);
                            char path[512];
                            snprintf(path, sizeof(path), "src/programs/%s", input_text);
                            loadAndInterpret(path, os_get_clock());
                            input_text[0] = '\0'; 
                        }
                    }
                }
            }
        }

        fill_rect(ren, (SDL_Rect){0, 0, 1200, 850}, C_BG);
        draw_text(ren, f_bold, 50, 30, C_ACCENT, "OSTOR OS DASHBOARD v3.7");
        
        draw_button(ren, f_reg, btn_rr, "RR", C_ACCENT, selected_algo == RR);
        draw_button(ren, f_reg, btn_hr, "HRRN", C_ACCENT, selected_algo == HRRN);
        draw_button(ren, f_reg, btn_ml, "MLFQ", C_ACCENT, selected_algo == MLFQ);
        draw_button(ren, f_reg, btn_step, "STEP", C_GREEN, false);

        Queue *ready_qs[4];
        int num_ready = 1;
        if (selected_algo == MLFQ) {
            ready_qs[0] = &mlfq_queues[0]; ready_qs[1] = &mlfq_queues[1];
            ready_qs[2] = &mlfq_queues[2]; ready_qs[3] = &mlfq_queues[3];
            num_ready = 4;
        } else {
            ready_qs[0] = &os_ready_queue;
        }
        
        draw_queue_visualized(ren, f_reg, f_sml, (SDL_Rect){50, 140, 530, 140}, "READY QUEUE", ready_qs, num_ready, C_GREEN);
        
        Queue *blocked_qs[1] = {&general_blocked_queue};
        draw_queue_visualized(ren, f_reg, f_sml, (SDL_Rect){620, 140, 530, 140}, "BLOCKED QUEUE", blocked_qs, 1, C_RED);

        SDL_Rect kernel_rect = {50, 290, 1100, 120};
        fill_rect(ren, kernel_rect, C_PANEL);
        stroke_rect(ren, kernel_rect, C_ACCENT);
        draw_text(ren, f_bold, 70, 310, C_ACCENT, "KERNEL EXECUTION");
        
        OSSnapshot snap = os_get_snapshot();
        char k_stat[128];
        if (snap.current_pid != -1) {
            snprintf(k_stat, sizeof(k_stat), "Running PID: %d | System Clock: %d", snap.current_pid, snap.clock_tick);
            draw_text(ren, f_reg, 70, 350, C_GREEN, k_stat);
        } else {
            snprintf(k_stat, sizeof(k_stat), "CPU Idle | System Clock: %d", snap.clock_tick);
            draw_text(ren, f_reg, 70, 350, C_TEXT, k_stat);
        }

        fill_rect(ren, term_rect, (SDL_Color){17, 17, 27, 255}); 
        stroke_rect(ren, term_rect, C_TEXT);

        int max_scroll = (log_count > VISIBLE_LINES) ? log_count - VISIBLE_LINES : 0;
        int base_idx = max_scroll - log_scroll_offset;
        if (base_idx < 0) base_idx = 0;

        for (int i = 0; i < VISIBLE_LINES && (base_idx + i) < log_count; i++) {
            draw_text(ren, f_sml, 70, 445 + (i * 28), C_TEXT, log_buffer[base_idx + i]);
        }
        
        if (max_scroll > 0) {
            char scroll_lbl[32];
            snprintf(scroll_lbl, sizeof(scroll_lbl), "Scroll: %d", log_scroll_offset);
            draw_text(ren, f_sml, 1050, 445, C_INACTIVE, scroll_lbl);
        }

        SDL_Color bg_color = input_focused ? C_PANEL : C_INACTIVE;
        fill_rect(ren, input_bar, bg_color);
        if (input_focused) stroke_rect(ren, input_bar, C_ACCENT); 
        
        SDL_Color prompt_col = is_waiting_for_input ? C_RED : C_YELLOW;
        const char* prompt_text = is_waiting_for_input ? "AWAITING INPUT > " : "FILE TO LOAD > ";
        
        draw_text(ren, f_reg, 70, 785, prompt_col, prompt_text);
        draw_text(ren, f_reg, 260, 785, C_TEXT, input_text);

        if (input_focused && (SDL_GetTicks() / 500) % 2) {
            int cur_x = 260 + (strlen(input_text) * 10); 
            fill_rect(ren, (SDL_Rect){cur_x, 785, 2, 20}, C_TEXT);
        }

        if (is_waiting_for_input) {
            SDL_Rect alert_box = {250, 350, 700, 120};
            
            if (!input_focused) {
                fill_rect(ren, alert_box, C_RED);
                stroke_rect(ren, alert_box, C_TEXT);
                draw_text(ren, f_bold, 280, 360, C_BG, "ACTION REQUIRED: AWAITING INPUT");
                draw_text(ren, f_reg, 280, 400, C_BG, alert_reason); 
                draw_text(ren, f_sml, 280, 430, C_BG, "-> Click the text box below, type your data, and press ENTER");
            } else {
                fill_rect(ren, alert_box, C_YELLOW);
                stroke_rect(ren, alert_box, C_TEXT);
                draw_text(ren, f_bold, 280, 360, C_BG, "INPUT MODE ACTIVE");
                draw_text(ren, f_reg, 280, 400, C_BG, alert_reason);
                draw_text(ren, f_sml, 280, 430, C_BG, "-> Type your data and press ENTER to submit");
            }
        }

        SDL_RenderPresent(ren);
    }

    TTF_CloseFont(f_sml); TTF_CloseFont(f_reg); TTF_CloseFont(f_bold);
    SDL_DestroyRenderer(ren); SDL_DestroyWindow(win);
    SDL_Quit(); return 0;
}