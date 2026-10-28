# matrixclear

A C11 port of [neoclear](https://codeberg.org/ftranschel/neoclear/), a `clear`
replacement that pours streams of braille glyphs (U+2800–U+28FF) in from the
screen edges. They meet in the middle, fade out, and leave a blank terminal,
scrollback included.

Unlike the Rust original there is no config file and no command-line arguments.
Every option is a compile-time constant in [`config.h`](config.h). No
dependencies beyond libc and libm; Linux only.

## Build

```sh
nix develop          # gcc + make (or direnv: `direnv allow` once)
make                 # builds target/release/matrixclear
alias clear=/path/to/cneoclear/target/release/matrixclear
```

Sources are in `src/`; [`config.h`](config.h) stays at the root because it is
the file you edit.

## Configure

Edit `config.h`, or override values per build:

```sh
make clean && make CPPFLAGS='-DDIR_LEFT=1 -DDIR_RIGHT=1 -DCOLOR=0xff2d2d'
```

| Macro                                         | Default             | Meaning                                                   |
| --------------------------------------------- | ------------------- | --------------------------------------------------------- |
| `DIR_TOP` `DIR_BOTTOM` `DIR_LEFT` `DIR_RIGHT` | `1 0 0 0`           | edges the streams start from                              |
| `RANDOM_DIRECTIONS`                           | `0`                 | pick a random combination of edges every run              |
| `DURATION_MS`                                 | `900`               | time until every stream reaches the meeting line          |
| `FADE_MS` / `FADE_STEPS`                      | `350` / `10`        | fade-out length and brightness steps                      |
| `FPS`                                         | `60`                | frames per second                                         |
| `COLOR`                                       | `0x1cff5a`          | stream colour                                             |
| `HEAD_COLOR_AUTO` / `HEAD_COLOR`              | `1` / `0xffffff`    | leading glyph: whitened tint of `COLOR`, or this colour   |
| `TRAIL` / `REST`                              | `10.0f` / `0.45f`   | bright-trail length, brightness behind it                 |
| `SPEED_SPREAD`                                | `3.0f`              | fastest/slowest stream speed ratio                        |
| `SPEED`                                       | `0.0f`              | rows/s of the slowest streams; 0 = fit into `DURATION_MS` |
| `STAGGER`                                     | `0.9f`              | how long fast streams may wait to start                   |
| `GLITCH`                                      | `0.02f`             | per-frame flicker chance of a settled glyph               |
| `CHARS_LO` / `CHARS_HI`                       | `0x2800` / `0x28FF` | code-point range                                          |
| `COLOR_MODE`                                  | `COLOR_MODE_AUTO`   | or `COLOR_MODE_TRUECOLOR`, `COLOR_MODE_ANSI256`           |
| `CLEAR_SCROLLBACK`                            | `1`                 | also erase scrollback, like `clear`                       |

Values outside the limits the original enforces fail to compile.

## Behaviour

- When stdout isn't a terminal it only emits the clear sequence, like `clear`.
- Ctrl-C, SIGTERM or a resize skips straight to the final clear; cursor, wrap
  mode and input echo are always restored.

`make sanitize` builds `target/debug/matrixclear` with AddressSanitizer and
UBSan.

## License

GPL-3.0-or-later, see [LICENSE](LICENSE).
