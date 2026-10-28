#ifndef MATRIXCLEAR_TERM_H
#define MATRIXCLEAR_TERM_H

#include <stdbool.h>
#include <stddef.h>

/* True once SIGINT/SIGTERM/SIGHUP arrived. */
bool term_interrupted(void);
bool term_stdout_is_tty(void);
/* Terminal size as columns and rows; false if unknown. */
bool term_size(size_t *cols, size_t *rows);
/* Installs signal handlers and turns off input echo (so keys pressed during
 * the animation don't scribble over it). */
void term_guard_init(void);
/* Restores echo; no-op if term_guard_init didn't change it. */
void term_guard_restore(void);

#endif
