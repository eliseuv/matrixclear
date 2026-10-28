/* The screen is partitioned between the enabled edges: every cell belongs to
 * the edge it is (proportionally) closest to, so one edge sweeps the whole
 * screen, two opposite edges meet on the centre line, and all four meet in an
 * X. Each column (or row) gets one stream with its own speed and start delay;
 * all streams are timed to arrive within DURATION_MS. Once every stream has
 * arrived the whole field fades to black. */

#include "anim.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "config.h"

#define NONE UINT32_MAX

enum dir { DIR_T, DIR_B, DIR_L, DIR_R };

static bool dir_vertical(enum dir d) {
    return d == DIR_T || d == DIR_B;
}

/* ------------------------------------------------------------------ RNG -- */

struct rng {
    uint64_t state;
};

static uint64_t rng_next(struct rng *r) {
    /* xorshift64* */
    uint64_t x = r->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    r->state = x;
    return x * UINT64_C(0x2545F4914F6CDD1D);
}

static uint64_t rotl64(uint64_t x, unsigned k) {
    return x << k | x >> (64 - k);
}

static struct rng rng_new(void) {
    struct timespec ts;
    uint64_t t = UINT64_C(0x9E3779B97F4A7C15);
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) t = (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
    unsigned char local = 0;
    uint64_t seed = t ^ ((uint64_t)getpid() << 32) ^ rotl64((uint64_t)(uintptr_t)&local, 17);
    struct rng r = {seed | 1};
    for (int i = 0; i < 4; i++) rng_next(&r);
    return r;
}

static float rng_f32(struct rng *r) {
    return (float)(rng_next(r) >> 40) / (float)(UINT64_C(1) << 24);
}

static uint32_t rng_range(struct rng *r, uint32_t lo, uint32_t hi) {
    return lo + (uint32_t)(rng_next(r) % ((uint64_t)hi - lo + 1));
}

/* ------------------------------------------------------------ animation -- */

struct lane {
    float delay; /* seconds before the stream enters */
    float speed; /* cells per second */
    uint32_t len; /* cells this stream must cover to reach the meeting line */
    float trail; /* bright-trail length in cells (longer for faster streams) */
};

struct anim {
    size_t w, ncells;
    bool truecolor;
    size_t nlanes;
    struct lane *lanes;
    float *heads;
    uint32_t *cell_lane; /* lane owning each cell (row-major) */
    uint16_t *cell_pos; /* distance of the cell from its lane's edge */
    uint32_t *glyph;
    float *shade; /* per-cell brightness variation, for uneven colours */
    uint32_t *drawn_key;
    uint32_t *drawn_ch;
    struct rng rng;
    float color[3];
    float head[3];
    float fade_s;
    bool done;
    float t_done;
};

static bool valid_scalar(uint32_t c) {
    return c <= 0x10FFFF && !(c >= 0xD800 && c <= 0xDFFF);
}

static uint32_t random_glyph(struct rng *r) {
    for (int i = 0; i < 8; i++) {
        uint32_t c = rng_range(r, CHARS_LO, CHARS_HI);
        if (valid_scalar(c)) return c;
    }
    for (uint32_t c = CHARS_LO; c <= CHARS_HI; c++)
        if (valid_scalar(c)) return c;
    return 0x28FF;
}

/* Nearest xterm-256 colour (6x6x6 cube or grey ramp). */
static int ansi_level(uint8_t v) {
    if (v < 48) return 0;
    if (v < 115) return 1;
    int q = (v - 35) / 40;
    return q < 5 ? q : 5;
}

static int sq(int x) {
    return x * x;
}

static uint8_t to_ansi256(const uint8_t rgb[3]) {
    static const int lv[6] = {0, 95, 135, 175, 215, 255};
    int r = rgb[0], g = rgb[1], b = rgb[2];
    int qr = ansi_level(rgb[0]), qg = ansi_level(rgb[1]), qb = ansi_level(rgb[2]);
    int cube = sq(lv[qr] - r) + sq(lv[qg] - g) + sq(lv[qb] - b);
    int avg = (r + g + b) / 3;
    int gi = (avg - 3 > 0 ? avg - 3 : 0) / 10;
    if (gi > 23) gi = 23;
    int gv = 8 + 10 * gi;
    if (sq(gv - r) + sq(gv - g) + sq(gv - b) < cube) return (uint8_t)(232 + gi);
    return (uint8_t)(16 + 36 * qr + 6 * qg + qb);
}

/* Normalised distance of a cell from an edge: 0 at the edge, 1 at the
 * opposite edge. Scaling by the screen dimension makes horizontal and
 * vertical streams arrive together and puts meeting lines on the
 * centre/diagonals. */
static float dist(enum dir d, size_t x, size_t y, size_t w, size_t h) {
    switch (d) {
    case DIR_T: return ((float)y + 0.5f) / (float)h;
    case DIR_B: return ((float)h - 0.5f - (float)y) / (float)h;
    case DIR_L: return ((float)x + 0.5f) / (float)w;
    case DIR_R: return ((float)w - 0.5f - (float)x) / (float)w;
    }
    return 0.0f;
}

static enum dir owner(const enum dir *enabled, size_t n, size_t x, size_t y, size_t w, size_t h) {
    enum dir best = enabled[0];
    for (size_t i = 1; i < n; i++)
        if (dist(enabled[i], x, y, w, h) < dist(best, x, y, w, h)) best = enabled[i];
    return best;
}

static uint32_t unpack(uint32_t rgb, int shift) {
    return rgb >> shift & 0xff;
}

void anim_free(struct anim *a) {
    if (!a) return;
    free(a->lanes);
    free(a->heads);
    free(a->cell_lane);
    free(a->cell_pos);
    free(a->glyph);
    free(a->shade);
    free(a->drawn_key);
    free(a->drawn_ch);
    free(a);
}

struct anim *anim_new(size_t w, size_t h, bool truecolor) {
    /* Callers bound w and h to 65535, which keeps cell_pos in range and
     * w * h far from overflowing. */
    struct anim *a = calloc(1, sizeof *a);
    if (!a) return NULL;
    size_t n = w * h;
    a->w = w;
    a->ncells = n;
    a->truecolor = truecolor;
    a->rng = rng_new();

    bool dirs[4] = {DIR_TOP, DIR_BOTTOM, DIR_LEFT, DIR_RIGHT};
    if (RANDOM_DIRECTIONS) {
        /* any of the 15 non-empty subsets of {top, bottom, left, right}, uniformly */
        uint32_t m = rng_range(&a->rng, 1, 15);
        for (int i = 0; i < 4; i++) dirs[i] = (m >> i & 1) != 0;
    }
    enum dir enabled[4];
    size_t nenabled = 0;
    for (int i = 0; i < 4; i++)
        if (dirs[i]) enabled[nenabled++] = (enum dir)i;

    size_t max_lanes = 0;
    for (size_t i = 0; i < nenabled; i++) max_lanes += dir_vertical(enabled[i]) ? w : h;

    a->lanes = malloc(max_lanes * sizeof *a->lanes);
    a->heads = calloc(max_lanes, sizeof *a->heads);
    a->cell_lane = malloc(n * sizeof *a->cell_lane);
    a->cell_pos = calloc(n, sizeof *a->cell_pos);
    a->glyph = malloc(n * sizeof *a->glyph);
    a->shade = malloc(n * sizeof *a->shade);
    a->drawn_key = malloc(n * sizeof *a->drawn_key);
    a->drawn_ch = calloc(n, sizeof *a->drawn_ch);
    enum dir *lane_dir = malloc(max_lanes * sizeof *lane_dir);
    if (!a->lanes || !a->heads || !a->cell_lane || !a->cell_pos || !a->glyph || !a->shade || !a->drawn_key ||
        !a->drawn_ch || !lane_dir) {
        free(lane_dir);
        anim_free(a);
        return NULL;
    }
    for (size_t i = 0; i < n; i++) a->cell_lane[i] = a->drawn_key[i] = NONE;

    for (size_t e = 0; e < nenabled; e++) {
        enum dir d = enabled[e];
        size_t count = dir_vertical(d) ? w : h, depth = dir_vertical(d) ? h : w;
        for (size_t lane = 0; lane < count; lane++) {
            uint32_t id = (uint32_t)a->nlanes;
            uint32_t len = 0;
            for (size_t p = 0; p < depth; p++) {
                size_t x, y;
                switch (d) {
                case DIR_T: x = lane, y = p; break;
                case DIR_B: x = lane, y = h - 1 - p; break;
                case DIR_L: x = p, y = lane; break;
                default: x = w - 1 - p, y = lane; break;
                }
                /* ownership along a lane is always a prefix starting at the edge */
                if (owner(enabled, nenabled, x, y, w, h) != d) break;
                a->cell_lane[y * w + x] = id;
                a->cell_pos[y * w + x] = (uint16_t)p;
                len++;
            }
            a->lanes[a->nlanes] = (struct lane){0.0f, 1.0f, len, TRAIL};
            lane_dir[a->nlanes] = d;
            a->nlanes++;
        }
    }

    /* Timing. `dur` is when the last stream must have arrived: either the
     * configured duration, or derived from an absolute SPEED (rows/s for the
     * slowest vertical streams; columns move twice as fast because terminal
     * cells are about twice as tall as wide). */
    uint32_t max_len[4] = {1, 1, 1, 1};
    for (size_t i = 0; i < a->nlanes; i++)
        if (a->lanes[i].len > max_len[lane_dir[i]]) max_len[lane_dir[i]] = a->lanes[i].len;
    float dur;
    if (SPEED > 0.0f) {
        dur = 0.0f;
        for (size_t i = 0; i < nenabled; i++) {
            enum dir d = enabled[i];
            float v = ((float)max_len[d] + 1.0f) / (dir_vertical(d) ? SPEED : 2.0f * SPEED);
            dur = fmaxf(dur, v);
        }
    } else {
        dur = (float)DURATION_MS / 1000.0f;
    }
    dur = fmaxf(dur, 0.01f);

    /* Each stream gets a speed factor f in [1, SPEED_SPREAD] (log-uniform):
     * the slowest streams need the whole `dur` to cross the longest lane of
     * their direction, the fastest only dur/spread. Faster streams start
     * later (random share of their slack), so everything still arrives by
     * `dur` while very different speeds are on screen at the same time. */
    float spread = fmaxf(SPEED_SPREAD, 1.0f);
    for (size_t i = 0; i < a->nlanes; i++) {
        struct lane *l = &a->lanes[i];
        float f = powf(spread, rng_f32(&a->rng));
        float travel = dur / f;
        l->delay = rng_f32(&a->rng) * STAGGER * (dur - travel);
        /* +1 so the head passes the last cell of the longest lane at `travel` */
        l->speed = ((float)max_len[lane_dir[i]] + 1.0f) / travel;
        l->trail = TRAIL * sqrtf(f);
    }
    free(lane_dir);

    static const float shades[4] = {0.72f, 0.84f, 0.92f, 1.0f};
    for (size_t i = 0; i < n; i++) a->glyph[i] = random_glyph(&a->rng);
    for (size_t i = 0; i < n; i++) a->shade[i] = shades[rng_next(&a->rng) % 4];

    for (int c = 0; c < 3; c++) {
        int shift = 16 - 8 * c;
        uint32_t base = unpack(COLOR, shift);
        a->color[c] = (float)base;
        a->head[c] = HEAD_COLOR_AUTO ? roundf((float)base + (255.0f - (float)base) * 0.8f)
                                     : (float)unpack(HEAD_COLOR, shift);
    }
    a->fade_s = (float)FADE_MS / 1000.0f;
    return a;
}

bool anim_render(struct anim *a, float t, struct buf *out) {
    bool all_arrived = true;
    for (size_t i = 0; i < a->nlanes; i++) {
        a->heads[i] = (t - a->lanes[i].delay) * a->lanes[i].speed;
        if (a->heads[i] < (float)a->lanes[i].len) all_arrived = false;
    }
    if (all_arrived && !a->done) {
        a->done = true;
        a->t_done = t;
    }
    float fade = 1.0f;
    if (a->done) {
        if (a->fade_s <= 0.0f) {
            /* No fade: still draw the frame in which the last streams arrive
             * (at full brightness), then finish on the next one. */
            if (t > a->t_done) return true;
        } else {
            float p = (t - a->t_done) / a->fade_s;
            if (p >= 1.0f) return true;
            /* stepped, slightly eased-in decrease of brightness */
            float steps = (float)FADE_STEPS;
            float step = floorf(p * steps) + 1.0f;
            fade = powf(1.0f - step / (steps + 1.0f), 1.3f);
        }
    }

    ptrdiff_t cursor = -1; /* index the terminal cursor sits on, -1 = unknown */
    uint32_t last_key = NONE;
    for (size_t idx = 0; idx < a->ncells; idx++) {
        uint32_t lane = a->cell_lane[idx];
        if (lane == NONE) continue;
        float head = a->heads[lane];
        float trail = a->lanes[lane].trail;
        float k = head - (float)a->cell_pos[idx]; /* cells behind the head */
        if (k < 0.0f) continue; /* not reached yet: the old screen content stays visible */
        float flicker = k < 1.0f ? 0.5f : GLITCH;
        if (rng_f32(&a->rng) < flicker) a->glyph[idx] = random_glyph(&a->rng);

        float level = REST + (1.0f - REST) * fmaxf(1.0f - k / trail, 0.0f);
        float white = fminf(fmaxf(1.0f - k / 1.5f, 0.0f), 1.0f);
        float s = level * a->shade[idx];
        uint8_t rgb[3];
        for (int c = 0; c < 3; c++) {
            float v = (a->color[c] * s) * (1.0f - white) + a->head[c] * white;
            /* clamp before converting: out-of-range float->int is UB in C */
            rgb[c] = (uint8_t)fminf(fmaxf(roundf(v * fade), 0.0f), 255.0f);
        }
        uint32_t key = a->truecolor ? (uint32_t)rgb[0] << 16 | (uint32_t)rgb[1] << 8 | rgb[2] : to_ansi256(rgb);
        uint32_t ch = a->glyph[idx];
        if (key == a->drawn_key[idx] && ch == a->drawn_ch[idx]) continue;
        if (cursor != (ptrdiff_t)idx) {
            buf_append_str(out, "\x1b[");
            buf_append_uint(out, idx / a->w + 1);
            buf_append_str(out, ";");
            buf_append_uint(out, idx % a->w + 1);
            buf_append_str(out, "H");
        }
        if (key != last_key) {
            if (a->truecolor) {
                buf_append_str(out, "\x1b[38;2;");
                buf_append_uint(out, rgb[0]);
                buf_append_str(out, ";");
                buf_append_uint(out, rgb[1]);
                buf_append_str(out, ";");
                buf_append_uint(out, rgb[2]);
            } else {
                buf_append_str(out, "\x1b[38;5;");
                buf_append_uint(out, key);
            }
            buf_append_str(out, "m");
            last_key = key;
        }
        buf_push_utf8(out, ch);
        a->drawn_key[idx] = key;
        a->drawn_ch[idx] = ch;
        /* after printing, the cursor advances unless we're in the last column */
        cursor = (idx + 1) % a->w != 0 ? (ptrdiff_t)(idx + 1) : -1;
    }
    return false;
}
