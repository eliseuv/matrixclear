#ifndef MATRIXCLEAR_CONFIG_H
#define MATRIXCLEAR_CONFIG_H

#define COLOR_MODE_AUTO 0
#define COLOR_MODE_TRUECOLOR 1
#define COLOR_MODE_ANSI256 2

/* Edges streams start from. */
#ifndef DIR_TOP
#define DIR_TOP 1
#endif
#ifndef DIR_BOTTOM
#define DIR_BOTTOM 0
#endif
#ifndef DIR_LEFT
#define DIR_LEFT 0
#endif
#ifndef DIR_RIGHT
#define DIR_RIGHT 0
#endif
/* Pick a random non-empty combination of edges on every run instead. */
#ifndef RANDOM_DIRECTIONS
#define RANDOM_DIRECTIONS 0
#endif

/* Time (ms) until every stream has reached the meeting line. */
#ifndef DURATION_MS
#define DURATION_MS 900
#endif
/* Length (ms) of the fade-out once all streams have arrived. */
#ifndef FADE_MS
#define FADE_MS 350
#endif
/* Number of discrete brightness steps in the fade. */
#ifndef FADE_STEPS
#define FADE_STEPS 10
#endif
#ifndef FPS
#define FPS 60
#endif

/* Body colour of the streams, 0xRRGGBB. */
#ifndef COLOR
#define COLOR 0x1cff5a
#endif
/* 1: the leading glyph uses COLOR mixed 80% towards white; 0: HEAD_COLOR. */
#ifndef HEAD_COLOR_AUTO
#define HEAD_COLOR_AUTO 1
#endif
#ifndef HEAD_COLOR
#define HEAD_COLOR 0xffffff
#endif

/* Trail length in cells over which brightness drops to REST. */
#ifndef TRAIL
#define TRAIL 10.0f
#endif
/* Brightness (0..1) of glyphs behind the trail. */
#ifndef REST
#define REST 0.45f
#endif
/* Ratio between the fastest and the slowest stream (1 = all equal). */
#ifndef SPEED_SPREAD
#define SPEED_SPREAD 3.0f
#endif
/* Absolute speed of the slowest streams in rows/s; 0 = derive from DURATION_MS. */
#ifndef SPEED
#define SPEED 0.0f
#endif
/* How much of its spare time (0..1) a fast stream may wait before starting. */
#ifndef STAGGER
#define STAGGER 0.9f
#endif
/* Per-frame probability that a settled glyph changes. */
#ifndef GLITCH
#define GLITCH 0.02f
#endif

/* Inclusive code-point range glyphs are drawn from. */
#ifndef CHARS_LO
#define CHARS_LO 0x2800
#endif
#ifndef CHARS_HI
#define CHARS_HI 0x28FF
#endif

#ifndef COLOR_MODE
#define COLOR_MODE COLOR_MODE_AUTO
#endif
/* Also wipe the scrollback buffer (ESC[3J), like `clear`. */
#ifndef CLEAR_SCROLLBACK
#define CLEAR_SCROLLBACK 1
#endif

#define CFG_MILLI(x) ((long)((x) * 1000.0))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wgnu-folding-constant"
#endif
_Static_assert(RANDOM_DIRECTIONS || DIR_TOP || DIR_BOTTOM || DIR_LEFT || DIR_RIGHT, "no direction enabled");
_Static_assert(DURATION_MS >= 0 && FADE_MS >= 0, "DURATION_MS and FADE_MS must be >= 0");
_Static_assert(FADE_STEPS >= 1, "FADE_STEPS must be >= 1");
_Static_assert(FPS >= 1 && FPS <= 1000, "FPS must be between 1 and 1000");
_Static_assert(COLOR >= 0 && COLOR <= 0xffffff && HEAD_COLOR >= 0 && HEAD_COLOR <= 0xffffff, "colours are 0xRRGGBB");
_Static_assert(CFG_MILLI(TRAIL) >= 1000, "TRAIL must be >= 1");
_Static_assert(CFG_MILLI(REST) >= 0 && CFG_MILLI(REST) <= 1000, "REST must be between 0 and 1");
_Static_assert(CFG_MILLI(SPEED_SPREAD) >= 1000 && CFG_MILLI(SPEED_SPREAD) <= 20000, "SPEED_SPREAD must be between 1 and 20");
_Static_assert(CFG_MILLI(SPEED) >= 0, "SPEED must be >= 0 (0 = use DURATION_MS)");
_Static_assert(CFG_MILLI(STAGGER) >= 0 && CFG_MILLI(STAGGER) <= 1000, "STAGGER must be between 0 and 1");
_Static_assert(CFG_MILLI(GLITCH) >= 0 && CFG_MILLI(GLITCH) <= 1000, "GLITCH must be between 0 and 1");
_Static_assert(CHARS_LO <= CHARS_HI && CHARS_HI <= 0x10FFFF, "unusable CHARS range");
_Static_assert(!(CHARS_LO >= 0xD800 && CHARS_HI <= 0xDFFF), "CHARS range contains only surrogates");
_Static_assert(COLOR_MODE == COLOR_MODE_AUTO || COLOR_MODE == COLOR_MODE_TRUECOLOR || COLOR_MODE == COLOR_MODE_ANSI256,
               "bad COLOR_MODE");
#pragma GCC diagnostic pop

#endif
