#ifndef MATRIXCLEAR_BUF_H
#define MATRIXCLEAR_BUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Growable byte buffer. Allocation failure aborts, like Rust's String with
 * panic = "abort". */
struct buf {
    char *data;
    size_t len, cap;
};

void buf_init(struct buf *b, size_t cap);
void buf_free(struct buf *b);
void buf_append(struct buf *b, const char *s, size_t n);
void buf_append_str(struct buf *b, const char *s);
void buf_append_uint(struct buf *b, unsigned long v);
/* Encode a Unicode scalar value as UTF-8. */
void buf_push_utf8(struct buf *b, uint32_t cp);

/* Write all of s to fd, retrying short writes and EINTR. */
bool write_all(int fd, const char *s, size_t n);

#endif
