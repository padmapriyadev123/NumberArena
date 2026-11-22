/* ui.h - Terminal interface helpers: colours, banners, safe input. */
#ifndef UI_H
#define UI_H
#include <stddef.h>

#define CLR_RESET  "\x1b[0m"
#define CLR_BOLD   "\x1b[1m"
#define CLR_RED    "\x1b[31m"
#define CLR_GREEN  "\x1b[32m"
#define CLR_YELLOW "\x1b[33m"
#define CLR_PURPLE "\x1b[35m"
#define CLR_CYAN   "\x1b[36m"

void ui_init(void);                 /* detect colour support (and enable on Windows 10+) */
void ui_set_color(int enabled);     /* force colour on/off (--no-color) */
const char *ui_c(const char *code); /* returns code, or "" when colour is off */
void ui_clear(void);
void ui_logo(void);
void ui_title(const char *title);   /* boxed section title */
void ui_rule(void);
void ui_pause(void);
void ui_trim(char *s);
int  ui_ci_equal(const char *a, const char *b);
int  ui_read_line(char *buf, size_t size);   /* 1 = ok, 0 = EOF */
int  ui_read_int(const char *prompt, int min, int max, int *out); /* 1 = ok, 0 = EOF */
int  ui_confirm(const char *prompt);         /* 1 = yes */
#endif
