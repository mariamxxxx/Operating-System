#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "../interpreter/parser.h"
#include "../os/os_core.h"
#include "../os/syscalls.h"
#include "../scheduler/scheduler.h"

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
    int start_x;
    int start_y;
    int end_x;
    int end_y;
    Uint32 start_ms;
    Uint32 end_ms;
} DispatchAnimation;

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

static void draw_text(SDL_Renderer *renderer,
                      TTF_Font *font,
                      int x,
                      int y,
                      SDL_Color color,
                      const char *text) {
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Rect dst;

    if (!font || !text) {
        return;
    }

    surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) {
        return;
    }

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    dst.x = x;
    dst.y = y;
    dst.w = surface->w;
    dst.h = surface->h;
    SDL_RenderCopy(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

static void draw_button(SDL_Renderer *renderer,
                        TTF_Font *font,
                        UIButton button,
                        int mouse_x,
                        int mouse_y,
                        SDL_Color base,
                        SDL_Color hover,
                        SDL_Color text_color) {
    SDL_Color fill = point_in_rect(mouse_x, mouse_y, button.rect) ? hover : base;
    fill_rect(renderer, button.rect, fill);
    stroke_rect(renderer, button.rect, (SDL_Color){12, 20, 36, 255});
    draw_text(renderer, font, button.rect.x + 14, button.rect.y + 10, text_color, button.label);
}

static void draw_value_card(SDL_Renderer *renderer,
                            TTF_Font *title_font,
                            TTF_Font *value_font,
                            SDL_Rect rect,
                            const char *title,
                            const char *value,
                            SDL_Color accent) {
    SDL_Rect stripe = {rect.x, rect.y, 8, rect.h};
    fill_rect(renderer, rect, (SDL_Color){41, 54, 80, 255});
    fill_rect(renderer, stripe, accent);
    stroke_rect(renderer, rect, (SDL_Color){85, 104, 140, 255});
    draw_text(renderer, title_font, rect.x + 18, rect.y + 14, (SDL_Color){174, 193, 226, 255}, title);
    draw_text(renderer, value_font, rect.x + 18, rect.y + 40, (SDL_Color){227, 238, 255, 255}, value);
}

static void draw_queue_card(SDL_Renderer *renderer,
                            TTF_Font *title_font,
                            TTF_Font *value_font,
                            SDL_Rect rect,
                            const char *title,
                            int value,
                            SDL_Color accent) {
    char value_text[24];
    snprintf(value_text, sizeof(value_text), "%d", value);
    draw_value_card(renderer, title_font, value_font, rect, title, value_text, accent);
}

static void draw_pid_blocks_in_queue(SDL_Renderer *renderer,
                                     TTF_Font *font,
                                     SDL_Rect rect,
                                     Queue *queue,
                                     SDL_Color block_color,
                                     SDL_Color text_color) {
    QueueNode *node;
    int idx = 0;
    int x;
    int y;
    int block_w;
    int block_h;
    int step_x;
    int step_y;
    int text_offset_y;
    int limit = 10;

    if (!queue) {
        return;
    }

    node = queue->head;
    block_w = 52;
    block_h = (rect.h < 86) ? 24 : 30;
    step_x = block_w + 8;
    step_y = block_h + 6;
    text_offset_y = (block_h == 24) ? 3 : 6;
    x = rect.x + 12;
    y = rect.y + ((rect.h < 86) ? 40 : 52);

    while (node && idx < limit) {
        SDL_Rect pid_block;
        char pid_text[24];

        pid_block.x = x;
        pid_block.y = y;
        pid_block.w = block_w;
        pid_block.h = block_h;

        fill_rect(renderer, pid_block, block_color);
        stroke_rect(renderer, pid_block, (SDL_Color){20, 32, 48, 255});
        snprintf(pid_text, sizeof(pid_text), "P%d", node->process->pcb->pid);
        draw_text(renderer, font, pid_block.x + 10, pid_block.y + text_offset_y, text_color, pid_text);

        x += step_x;
        if (x + block_w > rect.x + rect.w - 10) {
            x = rect.x + 12;
            y += step_y;
        }

        node = node->next;
        idx++;
    }

    if (queue->size == 0) {
        int empty_y = rect.y + rect.h / 2 - 8;
        if (empty_y < rect.y + 34) {
            empty_y = rect.y + 34;
        }
        draw_text(renderer, font, rect.x + 14, empty_y, (SDL_Color){102, 118, 143, 255}, "Empty");
    } else if (queue->size > limit) {
        char more_text[32];
        snprintf(more_text, sizeof(more_text), "+%d more", queue->size - limit);
        draw_text(renderer, font, rect.x + 14, rect.y + rect.h - 24, (SDL_Color){88, 104, 134, 255}, more_text);
    }
}

static void draw_queue_lane(SDL_Renderer *renderer,
                            TTF_Font *title_font,
                            TTF_Font *value_font,
                            SDL_Rect rect,
                            const char *title,
                            Queue *queue,
                            SDL_Color accent,
                            SDL_Color block_color,
                            SDL_Color text_color) {
    char count_text[24];
    SDL_Rect stripe = {rect.x, rect.y, 8, rect.h};

    fill_rect(renderer, rect, (SDL_Color){41, 54, 80, 255});
    fill_rect(renderer, stripe, accent);
    stroke_rect(renderer, rect, (SDL_Color){85, 104, 140, 255});
    draw_text(renderer, title_font, rect.x + 16, rect.y + 12, (SDL_Color){174, 193, 226, 255}, title);

    snprintf(count_text, sizeof(count_text), "%d", queue ? queue->size : 0);
    draw_text(renderer, value_font, rect.x + rect.w - 38, rect.y + 10, (SDL_Color){227, 238, 255, 255}, count_text);

    draw_pid_blocks_in_queue(renderer, title_font, rect, queue, block_color, text_color);
}

static int handle_button_click(SDL_Event *event, UIButton button) {
    if (event->type != SDL_MOUSEBUTTONDOWN || event->button.button != SDL_BUTTON_LEFT) {
        return 0;
    }
    return point_in_rect(event->button.x, event->button.y, button.rect);
}

static void build_program_path(const char *name, char *out, size_t out_size) {
    if (strchr(name, '/') || strchr(name, '\\')) {
        snprintf(out, out_size, "%s", name);
        return;
    }
    snprintf(out, out_size, "src/programs/%s", name);
}

static void load_due_programs(ProgramSpec *programs, int count) {
    int clock = os_get_clock();
    int i;

    for (i = 0; i < count; i++) {
        if (!programs[i].loaded && programs[i].arrival == clock) {
            char path[512];
            build_program_path(programs[i].name, path, sizeof(path));
            loadAndInterpret(path, programs[i].arrival);
            programs[i].loaded = 1;
        }
    }

    flush_pending_rr_process();
}

static int all_programs_loaded(const ProgramSpec *programs, int count) {
    int i;
    for (i = 0; i < count; i++) {
        if (!programs[i].loaded) {
            return 0;
        }
    }
    return 1;
}

static const char *algo_name(SchedulerAlgorithm algorithm) {
    switch (algorithm) {
        case RR: return "RR";
        case HRRN: return "HRRN";
        case MLFQ: return "MLFQ";
        default: return "Unknown";
    }
}

static int get_dispatch_source_lane(SchedulerAlgorithm algorithm) {
    int i;
    if (algorithm == MLFQ) {
        for (i = 0; i < 4; i++) {
            if (mlfq_queues[i].size > 0) {
                return i;
            }
        }
        return 0;
    }
    return 0;
}

static Queue *queue_for_lane(SchedulerAlgorithm algorithm, int lane_index) {
    if (algorithm == MLFQ) {
        if (lane_index >= 0 && lane_index < 4) {
            return &mlfq_queues[lane_index];
        }
        return NULL;
    }

    if (lane_index == 0) {
        return &os_ready_queue;
    }
    if (lane_index == 1) {
        return &general_blocked_queue;
    }
    return NULL;
}

static void reset_simulation(ProgramSpec *programs,
                             int count,
                             SchedulerAlgorithm algorithm,
                             bool *initialized,
                             int *last_dispatched_pid,
                             DispatchAnimation *anim) {
    int i;
    os_pause();
    os_init(algorithm);

    for (i = 0; i < count; i++) {
        programs[i].loaded = 0;
    }

    *initialized = true;
    *last_dispatched_pid = -1;
    anim->active = 0;
}

static void perform_single_tick(ProgramSpec *programs,
                                int count,
                                SchedulerAlgorithm algorithm,
                                DispatchAnimation *anim,
                                SDL_Rect queue_lanes[],
                                SDL_Rect kernel_rect,
                                int *last_dispatched_pid) {
    OSSnapshot snapshot;
    int source_lane;

    load_due_programs(programs, count);
    if (all_programs_loaded(programs, count) && os_is_idle()) {
        return;
    }

    source_lane = get_dispatch_source_lane(algorithm);
    os_step();
    snapshot = os_get_snapshot();

    if (snapshot.current_pid != -1 && snapshot.current_pid != *last_dispatched_pid) {
        anim->active = 1;
        anim->pid = snapshot.current_pid;
        anim->start_x = queue_lanes[source_lane].x + queue_lanes[source_lane].w / 2;
        anim->start_y = queue_lanes[source_lane].y + queue_lanes[source_lane].h;
        anim->end_x = kernel_rect.x + kernel_rect.w / 2;
        anim->end_y = kernel_rect.y + 44;
        anim->start_ms = SDL_GetTicks();
        anim->end_ms = anim->start_ms + 700;
        *last_dispatched_pid = snapshot.current_pid;
    }
}

int main(int argc, char **argv) {
    ProgramSpec programs[] = {
        {"Program_1.txt", 0, 0},
        {"Program_2.txt", 1, 0},
        {"Program_3.txt", 4, 0}
    };
    SchedulerAlgorithm selected_algo = HRRN;
    bool initialized = false;
    bool running = true;
    int mouse_x = 0;
    int mouse_y = 0;
    int last_dispatched_pid = -1;
    DispatchAnimation dispatch_anim = {0};

    UIButton init_button = {{36, 84, 130, 42}, "Init"};
    UIButton reset_button = {{176, 84, 130, 42}, "Reset"};
    UIButton step_button = {{316, 84, 110, 42}, "Step"};
    UIButton rr_chip = {{760, 86, 90, 38}, "RR"};
    UIButton hrrn_chip = {{858, 86, 90, 38}, "HRRN"};
    UIButton mlfq_chip = {{956, 86, 90, 38}, "MLFQ"};

    SDL_Rect queue_lanes[4] = {
        {70, 250, 500, 72},
        {610, 250, 500, 72},
        {70, 332, 500, 72},
        {610, 332, 500, 72}
    };
    SDL_Rect kernel_rect = {50, 475, 1100, 170};

    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font_regular;
    TTF_Font *font_small;
    TTF_Font *font_large;

    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() != 0) {
        printf("TTF_Init failed: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    window = SDL_CreateWindow("OS Simulator GUI (SDL2)",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              1180,
                              740,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    font_regular = TTF_OpenFont("C:/Windows/Fonts/segoeui.ttf", 18);
    font_small = TTF_OpenFont("C:/Windows/Fonts/segoeui.ttf", 15);
    font_large = TTF_OpenFont("C:/Windows/Fonts/segoeui.ttf", 26);
    if (!font_regular || !font_small || !font_large) {
        printf("Font load failed: %s\n", TTF_GetError());
        if (font_regular) TTF_CloseFont(font_regular);
        if (font_small) TTF_CloseFont(font_small);
        if (font_large) TTF_CloseFont(font_large);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    printf("SDL2 controls:\n");
    printf("  I: init\n");
    printf("  R: reset\n");
    printf("  1/2/3: select RR/HRRN/MLFQ\n");
    printf("  N: single step\n");
    printf("  ESC or window close: exit\n");

    while (running) {
        SDL_Event event;
        OSSnapshot snapshot = {0};
        int simulation_completed = 0;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }

            if (handle_button_click(&event, rr_chip)) {
                selected_algo = RR;
                if (initialized) {
                    reset_simulation(programs, 3, selected_algo, &initialized, &last_dispatched_pid, &dispatch_anim);
                }
            }
            if (handle_button_click(&event, hrrn_chip)) {
                selected_algo = HRRN;
                if (initialized) {
                    reset_simulation(programs, 3, selected_algo, &initialized, &last_dispatched_pid, &dispatch_anim);
                }
            }
            if (handle_button_click(&event, mlfq_chip)) {
                selected_algo = MLFQ;
                if (initialized) {
                    reset_simulation(programs, 3, selected_algo, &initialized, &last_dispatched_pid, &dispatch_anim);
                }
            }

            if (handle_button_click(&event, init_button)) {
                reset_simulation(programs, 3, selected_algo, &initialized, &last_dispatched_pid, &dispatch_anim);
            }

            if (initialized && handle_button_click(&event, reset_button)) {
                reset_simulation(programs, 3, selected_algo, &initialized, &last_dispatched_pid, &dispatch_anim);
            }

            if (initialized && handle_button_click(&event, step_button)) {
                perform_single_tick(programs,
                                    3,
                                    selected_algo,
                                    &dispatch_anim,
                                    queue_lanes,
                                    kernel_rect,
                                    &last_dispatched_pid);
            }

            if (event.type == SDL_KEYDOWN) {
                SDL_Keycode key = event.key.keysym.sym;

                if (key == SDLK_ESCAPE) running = false;

                if (key == SDLK_1) {
                    selected_algo = RR;
                    if (initialized) {
                        reset_simulation(programs, 3, selected_algo, &initialized, &last_dispatched_pid, &dispatch_anim);
                    }
                }
                if (key == SDLK_2) {
                    selected_algo = HRRN;
                    if (initialized) {
                        reset_simulation(programs, 3, selected_algo, &initialized, &last_dispatched_pid, &dispatch_anim);
                    }
                }
                if (key == SDLK_3) {
                    selected_algo = MLFQ;
                    if (initialized) {
                        reset_simulation(programs, 3, selected_algo, &initialized, &last_dispatched_pid, &dispatch_anim);
                    }
                }

                if (key == SDLK_i) {
                    reset_simulation(programs, 3, selected_algo, &initialized, &last_dispatched_pid, &dispatch_anim);
                }
                if (key == SDLK_r && initialized) {
                    reset_simulation(programs, 3, selected_algo, &initialized, &last_dispatched_pid, &dispatch_anim);
                }

                if (initialized && key == SDLK_n) {
                    perform_single_tick(programs,
                                        3,
                                        selected_algo,
                                        &dispatch_anim,
                                        queue_lanes,
                                        kernel_rect,
                                        &last_dispatched_pid);
                }
            }
        }

        SDL_GetMouseState(&mouse_x, &mouse_y);
        snapshot = initialized ? os_get_snapshot() : (OSSnapshot){0};
        simulation_completed = initialized && all_programs_loaded(programs, 3) && os_is_idle();

        SDL_SetRenderDrawColor(renderer, 16, 20, 32, 255);
        SDL_RenderClear(renderer);

        fill_rect(renderer, (SDL_Rect){20, 16, 1140, 700}, (SDL_Color){21, 27, 44, 255});
        stroke_rect(renderer, (SDL_Rect){20, 16, 1140, 700}, (SDL_Color){41, 56, 88, 255});

        draw_text(renderer, font_large, 36, 24, (SDL_Color){122, 204, 255, 255}, "OStor yarab's OS");
        draw_text(renderer, font_small, 36, 56, (SDL_Color){147, 168, 201, 255}, "Manual tick simulation mode");

        draw_button(renderer, font_regular, init_button, mouse_x, mouse_y,
                    (SDL_Color){58, 187, 137, 255}, (SDL_Color){76, 210, 156, 255}, (SDL_Color){8, 22, 20, 255});
        draw_button(renderer, font_regular, reset_button, mouse_x, mouse_y,
                    (SDL_Color){226, 163, 84, 255}, (SDL_Color){242, 184, 112, 255}, (SDL_Color){34, 24, 8, 255});
        draw_button(renderer, font_regular, step_button, mouse_x, mouse_y,
                    (SDL_Color){103, 156, 255, 255}, (SDL_Color){129, 175, 255, 255}, (SDL_Color){10, 20, 42, 255});

        {
            SDL_Rect *active_chip = NULL;
            fill_rect(renderer, rr_chip.rect, selected_algo == RR ? (SDL_Color){96, 175, 255, 255} : (SDL_Color){61, 78, 108, 255});
            fill_rect(renderer, hrrn_chip.rect, selected_algo == HRRN ? (SDL_Color){96, 175, 255, 255} : (SDL_Color){61, 78, 108, 255});
            fill_rect(renderer, mlfq_chip.rect, selected_algo == MLFQ ? (SDL_Color){96, 175, 255, 255} : (SDL_Color){61, 78, 108, 255});
            stroke_rect(renderer, rr_chip.rect, (SDL_Color){95, 116, 151, 255});
            stroke_rect(renderer, hrrn_chip.rect, (SDL_Color){95, 116, 151, 255});
            stroke_rect(renderer, mlfq_chip.rect, (SDL_Color){95, 116, 151, 255});
            draw_text(renderer, font_regular, rr_chip.rect.x + 26, rr_chip.rect.y + 9,
                      selected_algo == RR ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){215, 226, 246, 255}, "RR");
            draw_text(renderer, font_regular, hrrn_chip.rect.x + 18, hrrn_chip.rect.y + 9,
                      selected_algo == HRRN ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){215, 226, 246, 255}, "HRRN");
            draw_text(renderer, font_regular, mlfq_chip.rect.x + 17, mlfq_chip.rect.y + 9,
                      selected_algo == MLFQ ? (SDL_Color){255, 255, 255, 255} : (SDL_Color){215, 226, 246, 255}, "MLFQ");

            if (selected_algo == RR) active_chip = &rr_chip.rect;
            if (selected_algo == HRRN) active_chip = &hrrn_chip.rect;
            if (selected_algo == MLFQ) active_chip = &mlfq_chip.rect;

            if (active_chip) {
                SDL_Rect glow = {active_chip->x - 2, active_chip->y - 2, active_chip->w + 4, active_chip->h + 4};
                stroke_rect(renderer, glow, (SDL_Color){39, 132, 241, 255});
            }
        }

        {
            char clock_value[24];
            char pid_value[24];
            char state_value[32];
            int display_pid = simulation_completed ? -1 : snapshot.current_pid;

            snprintf(clock_value, sizeof(clock_value), "%d", snapshot.clock_tick);
            snprintf(pid_value, sizeof(pid_value), "%d", display_pid);
            snprintf(state_value, sizeof(state_value), "%s", initialized ? "READY" : "NOT INIT");
            if (simulation_completed) {
                snprintf(state_value, sizeof(state_value), "%s", "COMPLETED");
            }

            draw_value_card(renderer, font_small, font_regular, (SDL_Rect){36, 136, 260, 80}, "Clock", clock_value, (SDL_Color){84, 162, 255, 255});
            draw_value_card(renderer, font_small, font_regular, (SDL_Rect){312, 136, 260, 80}, "Running PID", pid_value, (SDL_Color){102, 210, 164, 255});
            draw_value_card(renderer, font_small, font_regular, (SDL_Rect){588, 136, 260, 80}, "Algorithm", algo_name(selected_algo), (SDL_Color){241, 189, 107, 255});
            draw_value_card(renderer, font_small, font_regular, (SDL_Rect){864, 136, 268, 80}, "State", state_value, (SDL_Color){180, 156, 255, 255});
        }

        if (selected_algo == MLFQ) {
            SDL_Rect mlfq_blocked_rect = {70, 414, 1040, 53};

            queue_lanes[0] = (SDL_Rect){70, 250, 500, 72};
            queue_lanes[1] = (SDL_Rect){610, 250, 500, 72};
            queue_lanes[2] = (SDL_Rect){70, 332, 500, 72};
            queue_lanes[3] = (SDL_Rect){610, 332, 500, 72};

            draw_queue_lane(renderer, font_small, font_regular, queue_lanes[0], "Queue 0", &mlfq_queues[0], (SDL_Color){80, 180, 255, 255}, (SDL_Color){147, 220, 255, 255}, (SDL_Color){16, 43, 68, 255});
            draw_queue_lane(renderer, font_small, font_regular, queue_lanes[1], "Queue 1", &mlfq_queues[1], (SDL_Color){104, 198, 167, 255}, (SDL_Color){160, 233, 205, 255}, (SDL_Color){17, 61, 48, 255});
            draw_queue_lane(renderer, font_small, font_regular, queue_lanes[2], "Queue 2", &mlfq_queues[2], (SDL_Color){245, 182, 97, 255}, (SDL_Color){255, 221, 170, 255}, (SDL_Color){83, 48, 14, 255});
            draw_queue_lane(renderer, font_small, font_regular, queue_lanes[3], "Queue 3", &mlfq_queues[3], (SDL_Color){233, 129, 118, 255}, (SDL_Color){255, 193, 186, 255}, (SDL_Color){85, 35, 31, 255});
            draw_queue_lane(renderer, font_small, font_regular, mlfq_blocked_rect, "Blocked Queue", &general_blocked_queue, (SDL_Color){232, 123, 109, 255}, (SDL_Color){250, 184, 174, 255}, (SDL_Color){78, 31, 27, 255});
        } else {
            SDL_Rect ready_rect = {120, 250, 440, 140};
            SDL_Rect blocked_rect = {620, 250, 440, 140};

            queue_lanes[0] = ready_rect;
            queue_lanes[1] = blocked_rect;

            draw_queue_lane(renderer, font_small, font_regular, ready_rect, "Ready Queue", &os_ready_queue, (SDL_Color){98, 173, 255, 255}, (SDL_Color){149, 210, 255, 255}, (SDL_Color){16, 43, 68, 255});
            draw_queue_lane(renderer, font_small, font_regular, blocked_rect, "Blocked Queue", &general_blocked_queue, (SDL_Color){232, 123, 109, 255}, (SDL_Color){250, 184, 174, 255}, (SDL_Color){78, 31, 27, 255});
        }

        fill_rect(renderer, kernel_rect, (SDL_Color){20, 39, 34, 255});
        stroke_rect(renderer, kernel_rect, (SDL_Color){73, 198, 155, 255});
        draw_text(renderer, font_large, kernel_rect.x + 24, kernel_rect.y + 18, (SDL_Color){120, 245, 209, 255}, "Kernel");

        {
            int kernel_pid = simulation_completed ? -1 : snapshot.current_pid;

            if (kernel_pid != -1) {
                SDL_Rect running_block = {kernel_rect.x + 34, kernel_rect.y + 72, 140, 64};
                SDL_Rect glow = {running_block.x - 4, running_block.y - 4, running_block.w + 8, running_block.h + 8};
                char kernel_pid_text[32];

                fill_rect(renderer, glow, (SDL_Color){26, 86, 70, 255});
                fill_rect(renderer, running_block, (SDL_Color){125, 229, 193, 255});
                stroke_rect(renderer, running_block, (SDL_Color){19, 72, 58, 255});
                snprintf(kernel_pid_text, sizeof(kernel_pid_text), "PID %d", kernel_pid);
                draw_text(renderer, font_regular, running_block.x + 28, running_block.y + 20, (SDL_Color){16, 59, 45, 255}, kernel_pid_text);
                draw_text(renderer, font_small, running_block.x + 176, running_block.y + 22, (SDL_Color){205, 246, 234, 255}, "Executing in kernel");
            } else {
                draw_text(renderer, font_regular, kernel_rect.x + 30, kernel_rect.y + 88, (SDL_Color){215, 247, 239, 255}, "Kernel idle");
            }
        }

        {
            int i;
            int lane_count = (selected_algo == MLFQ) ? 0 : 2;
            SDL_SetRenderDrawColor(renderer, 69, 106, 168, 255);
            for (i = 0; i < lane_count; i++) {
                int queue_anchor_x = queue_lanes[i].x + queue_lanes[i].w / 2;
                int queue_anchor_y = queue_lanes[i].y + queue_lanes[i].h;
                int kernel_anchor_x = kernel_rect.x + kernel_rect.w / 2;
            int kernel_anchor_y = kernel_rect.y;

                SDL_RenderDrawLine(renderer,
                                   kernel_anchor_x,
                                   kernel_anchor_y,
                                   queue_anchor_x,
                                   queue_anchor_y);
            }

            if (selected_algo != MLFQ) {
                int ready_anchor_x = queue_lanes[0].x + queue_lanes[0].w;
                int ready_anchor_y = queue_lanes[0].y + queue_lanes[0].h / 2;
                int blocked_anchor_x = queue_lanes[1].x;
                int blocked_anchor_y = queue_lanes[1].y + queue_lanes[1].h / 2;

                SDL_RenderDrawLine(renderer,
                                   ready_anchor_x,
                                   ready_anchor_y,
                                   blocked_anchor_x,
                                   blocked_anchor_y);
            }
        }

        if (dispatch_anim.active) {
            Uint32 now = SDL_GetTicks();
            if (now > dispatch_anim.end_ms) {
                dispatch_anim.active = 0;
            } else {
                float t = (float)(now - dispatch_anim.start_ms) / (float)(dispatch_anim.end_ms - dispatch_anim.start_ms);
                int x;
                int y;
                SDL_Rect token;
                char pid_text[16];

                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;

                x = (int)(dispatch_anim.start_x + t * (dispatch_anim.end_x - dispatch_anim.start_x));
                y = (int)(dispatch_anim.start_y + t * (dispatch_anim.end_y - dispatch_anim.start_y));
                token.x = x - 12;
                token.y = y - 12;
                token.w = 24;
                token.h = 24;

                fill_rect(renderer, token, (SDL_Color){120, 215, 255, 255});
                stroke_rect(renderer, token, (SDL_Color){18, 64, 96, 255});
                snprintf(pid_text, sizeof(pid_text), "%d", dispatch_anim.pid);
                draw_text(renderer, font_small, x - 5, y - 8, (SDL_Color){14, 34, 49, 255}, pid_text);
            }
        }

        draw_text(renderer,
                  font_small,
                  36,
                  706,
                  (SDL_Color){121, 143, 183, 255},
                  "Controls: I init, R reset, N step | top-right chips choose scheduler");

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font_regular);
    TTF_CloseFont(font_small);
    TTF_CloseFont(font_large);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
