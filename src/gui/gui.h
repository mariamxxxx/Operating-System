#ifndef GUI_H
#define GUI_H

#include "../process/processs.h"
#include "../scheduler/scheduler.h"

typedef enum {
    GUI_ACTION_NONE,
    GUI_ACTION_STEP,
    GUI_ACTION_RUN,
    GUI_ACTION_PAUSE,
    GUI_ACTION_QUIT
} GuiAction;

typedef struct {
    GuiAction action;
    int run_steps;
} GuiControl;

typedef struct {
    int time_slice;
    int queue_level;
} GuiTimesliceInfo;

void gui_init(void);
void gui_shutdown(void);
int gui_is_initialized(void);
void gui_reset_state(void);
char *gui_prompt_input(const char *prompt);
char *gui_prompt_input_for_pid(const char *prompt, int pid);

GuiControl gui_get_control(void);

void gui_note_event(const char *event);
void gui_note_swap(int pid, const char *action);
void gui_log_output(const char *line);
void gui_print_queues(const char *event);

void gui_render_tick(const Process *running, const char *instruction, GuiTimesliceInfo slice);
void gui_render_idle(void);

#endif
