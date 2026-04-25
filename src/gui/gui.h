#ifndef GUI_H
#define GUI_H

/* gui_log: write a formatted message to the GUI log file (gui_log.txt).
 * Call this alongside printf() so that output goes to both the terminal
 * and the GUI log simultaneously. */
void gui_log(const char *fmt, ...);

/* Open / close the GUI log file.  gui_log_init() is called once at
 * start-up and gui_log_close() is called on exit. */
void gui_log_init(void);
void gui_log_close(void);

#endif
