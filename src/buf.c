#include "buf.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void buf_init(struct buf *b, size_t cap) {
    b->len = 0;
    b->cap = cap ? cap : 64;
    b->data = malloc(b->cap);
    if (!b->data) abort();
}

void buf_free(struct buf *b) {
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

static void buf_reserve(struct buf *b, size_t extra) {
    if (b->cap - b->len >= extra) return;
    size_t need = b->len + extra;
    if (need < b->len) abort();
    size_t cap = b->cap;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) {
            cap = need;
            break;
        }
        cap *= 2;
    }
    char *p = realloc(b->data, cap);
    if (!p) abort();
    b->data = p;
    b->cap = cap;
}

void buf_append(struct buf *b, const char *s, size_t n) {
    buf_reserve(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
}

void buf_append_str(struct buf *b, const char *s) {
    buf_append(b, s, strlen(s));
}

/* Hand-rolled because snprintf per cell dominates a full-screen frame. */
void buf_append_uint(struct buf *b, unsigned long v) {
    char tmp[20];
    size_t n = 0;
    do {
        tmp[sizeof tmp - 1 - n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    buf_append(b, tmp + sizeof tmp - n, n);
}

void buf_push_utf8(struct buf *b, uint32_t cp) {
    char t[4];
    size_t n;
    if (cp < 0x80) {
        t[0] = (char)cp;
        n = 1;
    } else if (cp < 0x800) {
        t[0] = (char)(0xC0 | cp >> 6);
        t[1] = (char)(0x80 | (cp & 0x3F));
        n = 2;
    } else if (cp < 0x10000) {
        t[0] = (char)(0xE0 | cp >> 12);
        t[1] = (char)(0x80 | (cp >> 6 & 0x3F));
        t[2] = (char)(0x80 | (cp & 0x3F));
        n = 3;
    } else {
        t[0] = (char)(0xF0 | cp >> 18);
        t[1] = (char)(0x80 | (cp >> 12 & 0x3F));
        t[2] = (char)(0x80 | (cp >> 6 & 0x3F));
        t[3] = (char)(0x80 | (cp & 0x3F));
        n = 4;
    }
    buf_append(b, t, n);
}

bool write_all(int fd, const char *s, size_t n) {
    while (n > 0) {
        ssize_t r = write(fd, s, n);
        if (r < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        s += r;
        n -= (size_t)r;
    }
    return true;
}
