#ifndef MATRIXCLEAR_ANIM_H
#define MATRIXCLEAR_ANIM_H

#include <stdbool.h>
#include <stddef.h>

#include "buf.h"

struct anim;

/* Lays out the streams for a w x h screen. Returns NULL on allocation failure. */
struct anim *anim_new(size_t w, size_t h, bool truecolor);
/* Append the escape sequences for frame `t` (seconds since start) to `out`.
 * Returns true once the fade-out has completed. */
bool anim_render(struct anim *a, float t, struct buf *out);
void anim_free(struct anim *a);

#endif
