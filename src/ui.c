/* ui.c - Terminal interface helpers. Uses plain ASCII so it works in any terminal. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include "ui.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

static int g_color = 0;

void ui_set_color(int enabled) { g_color = enabled ? 1 : 0; }

void ui_init(void)
{
    g_color = 0;
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    /* 0x0004 = ENABLE_VIRTUAL_TERMINAL_PROCESSING (Windows 10+) */
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode) &&
        SetConsoleMode(h, mode | 0x0004))
        g_color = 1;
#else
    const char *term = getenv("TERM");
    if (isatty(1) && term && strcmp(term, "dumb") != 0) g_color = 1;
#endif
    if (getenv("NO_COLOR")) g_color = 0;
}

const char *ui_c(const char *code) { return g_color ? code : ""; }

void ui_clear(void)
{
    if (g_color) printf("\x1b[2J\x1b[H");
    else printf("\n\n");
}

void ui_rule(void)
{
    printf("%s+------------------------------------------------------------------+%s\n",
           ui_c(CLR_CYAN), ui_c(CLR_RESET));
}

void ui_title(const char *title)
{
    char line[80];
    snprintf(line, sizeof line, "%.62s", title);
    ui_rule();
    printf("%s|%s %-64s %s|%s\n", ui_c(CLR_CYAN), ui_c(CLR_BOLD), line,
           ui_c(CLR_CYAN), ui_c(CLR_RESET));
    ui_rule();
}

void ui_logo(void)
{
    static const char *art[] = {
        " _   _ _   _ __  __ ____  _____ ____      _    ____  _____ _   _    _    ",
        "| \\ | | | | |  \\/  | __ )| ____|  _ \\    / \\  |  _ \\| ____| \\ | |  / \\   ",
        "|  \\| | | | | |\\/| |  _ \\|  _| | |_) |  / _ \\ | |_) |  _| |  \\| | / _ \\  ",
        "| |\\  | |_| | |  | | |_) | |___|  _ <  / ___ \\|  _ <| |___| |\\  |/ ___ \\ ",
        "|_| \\_|\\___/|_|  |_|____/|_____|_| \\_\\/_/   \\_\\_| \\_\\_____|_| \\_/_/   \\_\\"
    };
    size_t i;
    printf("%s", ui_c(CLR_CYAN));
    for (i = 0; i < sizeof art / sizeof art[0]; i++) printf("%s\n", art[i]);
    printf("%s%s      >>> ULTIMATE CODE BREAKER CHAMPIONSHIP <<<%s\n",
           ui_c(CLR_PURPLE), ui_c(CLR_BOLD), ui_c(CLR_RESET));
}

void ui_trim(char *s)
{
    size_t len = strlen(s), start = 0;
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    while (s[start] && isspace((unsigned char)s[start])) start++;
    if (start > 0) memmove(s, s + start, strlen(s + start) + 1);
}

int ui_ci_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

/* Reads one line safely: never overflows buf, discards any excess characters. */
int ui_read_line(char *buf, size_t size)
{
    size_t len;
    fflush(stdout);
    if (size == 0 || !fgets(buf, (int)size, stdin)) return 0;
    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') buf[--len] = '\0';
    else { int c; while ((c = getchar()) != '\n' && c != EOF) { } }
    ui_trim(buf);
    return 1;
}

int ui_read_int(const char *prompt, int min, int max, int *out)
{
    char line[64], *end;
    long v;
    for (;;) {
        printf("%s", prompt);
        if (!ui_read_line(line, sizeof line)) return 0;
        if (line[0] == '\0') { printf("%sPlease enter a number.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET)); continue; }
        errno = 0;
        v = strtol(line, &end, 10);
        if (*end != '\0' || errno == ERANGE || v < INT_MIN || v > INT_MAX) {
            printf("%sInvalid input. Digits only, please.%s\n", ui_c(CLR_RED), ui_c(CLR_RESET));
            continue;
        }
        if (v < min || v > max) {
            printf("%sEnter a value from %d to %d.%s\n", ui_c(CLR_RED), min, max, ui_c(CLR_RESET));
            continue;
        }
        *out = (int)v;
        return 1;
    }
}

int ui_confirm(const char *prompt)
{
    char line[16];
    printf("%s", prompt);
    if (!ui_read_line(line, sizeof line)) return 0;
    return ui_ci_equal(line, "y") || ui_ci_equal(line, "yes");
}

void ui_pause(void)
{
    char line[16];
    printf("\nPress ENTER to continue...");
    ui_read_line(line, sizeof line);
}
