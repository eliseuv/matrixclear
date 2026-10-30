#ifndef MATRIXCLEAR_ANIM_H
#define MATRIXCLEAR_ANIM_H

#include <stdbool.h>
#include <stddef.h>

#include "buf.h"

struct anim;

/* Lays out the streams for a w x h screen. Returns NULL on allocation failure. */
struct anim *anim_new(size_t w, size_t h, bool truecolor);
/* Append the escape sequences for frame `t` (seconds since start) to `out`.
 * Returns true once the streams have left the screen. */
bool anim_render(struct anim *a, float t, struct buf *out);
/* Whether this frame is moving the filled streams off the screen. */
bool anim_exiting(const struct anim *a, float t);
void anim_free(struct anim *a);

#endif
