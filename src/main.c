#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "anim.h"
#include "buf.h"
#include "config.h"
#include "term.h"

/* cell_pos is 16-bit; terminals report sizes as 16-bit anyway, only the
 * COLUMNS/LINES fallback could exceed this. */
#define MAX_DIM 65535

static void append_final_clear(struct buf *b) {
    buf_append_str(b, "\x1b[0m\x1b[H\x1b[2J");
    if (CLEAR_SCROLLBACK) buf_append_str(b, "\x1b[3J");
}

/* Case-insensitive substring test. */
static bool contains_ci(const char *haystack, const char *needle) {
    size_t n = strlen(needle);
    for (; *haystack; haystack++) {
        size_t i = 0;
        while (i < n && haystack[i] && tolower((unsigned char)haystack[i]) == needle[i]) i++;
        if (i == n) return true;
    }
    return n == 0;
}

static bool env_contains_any(const char *var, const char *const *needles) {
    const char *v = getenv(var);
    if (!v) return false;
    for (; *needles; needles++)
        if (contains_ci(v, *needles)) return true;
    return false;
}

static bool detect_truecolor(void) {
    static const char *const colorterm[] = {"truecolor", "24bit", NULL};
    static const char *const term[] = {"direct", "kitty", "alacritty", "wezterm", "foot", "ghostty", "contour", NULL};
    static const char *const term_program[] = {"wezterm", "vscode", "hyper", "ghostty", "tabby", NULL};
    return env_contains_any("COLORTERM", colorterm) || env_contains_any("TERM", term) ||
           env_contains_any("TERM_PROGRAM", term_program);
}

static double elapsed_s(const struct timespec *start) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (double)(now.tv_sec - start->tv_sec) + (double)(now.tv_nsec - start->tv_nsec) / 1e9;
}

static struct timespec add_ns(const struct timespec *base, uint64_t ns) {
    struct timespec r = *base;
    r.tv_sec += (time_t)(ns / 1000000000u);
    r.tv_nsec += (long)(ns % 1000000000u);
    if (r.tv_nsec >= 1000000000L) {
        r.tv_sec++;
        r.tv_nsec -= 1000000000L;
    }
    return r;
}

static int plain_clear(void) {
    struct buf b;
    buf_init(&b, 32);
    append_final_clear(&b);
    write_all(STDOUT_FILENO, b.data, b.len);
    buf_free(&b);
    return 0;
}

int main(void) {
    size_t w = 0, h = 0;
    if (!term_stdout_is_tty() || !term_size(&w, &h) || w == 0 || h == 0 || w > MAX_DIM || h > MAX_DIM) {
        /* Not a terminal (or unknown size): behave like plain `clear`. */
        return plain_clear();
    }

    term_guard_init();
    bool truecolor = COLOR_MODE == COLOR_MODE_TRUECOLOR || (COLOR_MODE == COLOR_MODE_AUTO && detect_truecolor());
    struct anim *anim = anim_new(w, h, truecolor);
    if (!anim) {
        term_guard_restore();
        return plain_clear();
    }

    /* Hide the cursor, disable auto-wrap, reset attributes. Each frame saves
     * the cursor position (DECSC) and ends by restoring it (DECRC) inside its
     * synchronized update, so the terminal only ever renders the cursor where
     * it started; drawing jumps around invisibly in between. (Saving per
     * frame also works on terminals that treat DECSC/DECRC as a one-shot
     * stack.) This keeps cursor shaders/trails (Ghostty etc.) still until the
     * single move to the top-left in the final clear. */
    static const char setup[] = "\x1b[?25l\x1b[?7l\x1b[0m";
    write_all(STDOUT_FILENO, setup, sizeof setup - 1);

    struct timespec start;
    clock_gettime(CLOCK_MONOTONIC, &start);
    struct buf b;
    buf_init(&b, w * h * 24);
    for (uint64_t frame = 1;; frame++) {
        size_t cw, ch;
        if (term_interrupted() || !term_size(&cw, &ch) || cw != w || ch != h)
            break; /* Ctrl-C or resize: skip straight to the clear */
        float t = (float)elapsed_s(&start);
        b.len = 0;
        buf_append_str(&b, "\x1b[?2026h\x1b" "7"); /* begin synchronized update, save cursor */
        size_t empty = b.len;
        bool finished = anim_render(anim, t, &b);
        if (b.len > empty) {
            buf_append_str(&b, "\x1b" "8\x1b[?2026l"); /* cursor back to where it started, then present */
            if (!write_all(STDOUT_FILENO, b.data, b.len)) break;
        }
        if (finished) break;
        /* Absolute deadlines don't drift; EINTR (Ctrl-C) ends the sleep early
         * and the loop re-checks the flag. */
        struct timespec next = add_ns(&start, frame * 1000000000u / FPS);
        while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR && !term_interrupted()) {
        }
    }

    /* the one and only visible cursor move: clear, home, show cursor */
    b.len = 0;
    buf_append_str(&b, "\x1b[?2026h");
    append_final_clear(&b);
    buf_append_str(&b, "\x1b[?7h\x1b[?25h\x1b[?2026l");
    write_all(STDOUT_FILENO, b.data, b.len);

    buf_free(&b);
    anim_free(anim);
    term_guard_restore();
    return 0;
}
