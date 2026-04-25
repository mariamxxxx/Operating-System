#include <stdio.h>
#include <stdarg.h>
#include "gui.h"

#define GUI_LOG_FILE "gui_log.txt"

static FILE *g_log_file = NULL;

void gui_log_init(void) {
    if (g_log_file != NULL) {
        return;
    }
    g_log_file = fopen(GUI_LOG_FILE, "w");
    if (g_log_file == NULL) {
        fprintf(stderr, "[gui] WARNING: could not open %s for writing\n", GUI_LOG_FILE);
    }
}

void gui_log_close(void) {
    if (g_log_file != NULL) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
}

void gui_log(const char *fmt, ...) {
    if (g_log_file == NULL) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(g_log_file, fmt, args);
    va_end(args);
}
