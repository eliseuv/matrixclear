/* Terminal plumbing for Linux: tty detection, window size, echo suppression
 * and signal handling. */

#include "term.h"

#ifndef __linux__
#error "neoclear supports Linux only"
#endif

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static volatile sig_atomic_t interrupted = 0;
static struct termios saved_termios;
static bool termios_saved = false;

bool term_interrupted(void) {
    return interrupted != 0;
}

bool term_stdout_is_tty(void) {
    return isatty(STDOUT_FILENO) == 1;
}

static bool env_size(const char *name, size_t *out) {
    const char *s = getenv(name);
    if (!s) return false;
    while (*s == ' ' || *s == '\t') s++;
    errno = 0;
    char *end;
    long v = strtol(s, &end, 10);
    while (*end == ' ' || *end == '\t' || *end == '\n') end++;
    if (errno || end == s || *end != '\0' || v < 0) return false;
    *out = (size_t)v;
    return true;
}

bool term_size(size_t *cols, size_t *rows) {
    static const int fds[] = {STDOUT_FILENO, STDERR_FILENO, STDIN_FILENO};
    for (size_t i = 0; i < sizeof fds / sizeof fds[0]; i++) {
        struct winsize ws;
        if (ioctl(fds[i], TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
            *cols = ws.ws_col;
            *rows = ws.ws_row;
            return true;
        }
    }
    size_t c, l;
    if (!env_size("COLUMNS", &c) || !env_size("LINES", &l)) return false;
    *cols = c;
    *rows = l;
    return true;
}

static void on_signal(int sig) {
    (void)sig;
    interrupted = 1;
}

void term_guard_init(void) {
    /* No SA_RESTART: the frame sleep should return early on Ctrl-C. */
    struct sigaction sa = {0};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (isatty(STDIN_FILENO) != 1 || tcgetattr(STDIN_FILENO, &saved_termios) != 0) return;
    struct termios t = saved_termios;
    t.c_lflag &= ~(tcflag_t)ECHO;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &t) == 0) termios_saved = true;
}

void term_guard_restore(void) {
    if (termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
        termios_saved = false;
    }
}
