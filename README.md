# matrixclear

A C11 port of [neoclear](https://codeberg.org/ftranschel/neoclear/), a `clear`
replacement that pours streams of Matrix-style glyphs in from the screen
edges. They meet in the middle, run off the screen, and leave a blank terminal,
scrollback included.

The screen is fully covered for one frame before the streams start leaving.
The departing streams erase the screen as they go; a completed animation does
not perform another full-screen clear.

The default glyphs mix halfwidth Katakana, digits and Latin letters. Halfwidth
characters fit the terminal's single-cell streams. Set `MATRIX_GLYPHS=0` to
restore the configurable code-point range (Braille by default).

Unlike the Rust original there is no config file and no command-line arguments.
Every option is a compile-time constant in [`config.h`](config.h). No
dependencies beyond libc and libm; Linux only.

## Build

```sh
nix develop          # gcc + make (or direnv: `direnv allow` once)
make                 # builds target/release/matrixclear
MATRIXCLEAR_BIN="$PWD/target/release/matrixclear"
source "$PWD/shell/matrixclear.zsh"  # in zsh; add absolute paths to ~/.zshrc
```

Sources are in `src/`; [`config.h`](config.h) stays at the root because it is
the file you edit.

The zsh function replaces an existing `clear` alias and passes the expanded
left prompt to the renderer. Its Ctrl-L widget also preserves the command being
edited. This lets the prompt appear while the glyphs leave, before zsh redraws
it normally. Run the binary directly for the plain animation without prompt
preview.

If tmux uses `vim-tmux-navigator`, its default Ctrl+L binding intercepts the
key before zsh receives it. Set `@vim_navigator_mapping_right` to `C-Right`
before loading that plugin so the Ctrl+L widget works inside tmux panes.

Inside tmux, frames are sent without synchronized output: tmux would discard
whole synchronized frames as too large and freeze the pane, so it may show
partial frames instead.

## Install with Nix

```sh
nix run github:<owner>/matrixclear      # try it
```

As a flake input, use either `inputs.matrixclear.packages.${system}.default`
or `inputs.matrixclear.overlays.default` (adds `pkgs.matrixclear`). Since
options are compile-time, set them with `override`:

```nix
pkgs.matrixclear.override { defines = [ "DIR_LEFT=1" "COLOR=0xff2d2d" ]; }
```

## Configure

Edit `config.h`, or override values per build:

```sh
make clean && make CPPFLAGS='-DDIR_LEFT=1 -DDIR_RIGHT=1 -DCOLOR=0xff2d2d'
```

| Macro                                         | Default             | Meaning                                                   |
| --------------------------------------------- | ------------------- | --------------------------------------------------------- |
| `DIR_TOP` `DIR_BOTTOM` `DIR_LEFT` `DIR_RIGHT` | `1 0 0 0`           | edges the streams start from                              |
| `RANDOM_DIRECTIONS`                           | `0`                 | pick a random combination of edges every run              |
| `DURATION_MS`                                 | `500`               | time until every stream reaches the meeting line          |
| `EXIT_MS`                                     | `500`               | time for streams to run out after they arrive             |
| `FPS`                                         | `60`                | frames per second                                         |
| `COLOR`                                       | `0x1cff5a`          | stream colour                                             |
| `HEAD_COLOR_AUTO` / `HEAD_COLOR`              | `1` / `0xffffff`    | leading glyph: whitened tint of `COLOR`, or this colour   |
| `TRAIL` / `REST`                              | `10.0f` / `0.45f`   | bright-trail length, brightness behind it                 |
| `SPEED_SPREAD`                                | `3.0f`              | fastest/slowest stream speed ratio                        |
| `SPEED`                                       | `0.0f`              | rows/s of the slowest streams; 0 = fit into `DURATION_MS` |
| `STAGGER`                                     | `0.9f`              | how long fast streams may wait to start                   |
| `GLITCH`                                      | `0.02f`             | per-frame flicker chance of a settled glyph               |
| `MATRIX_GLYPHS`                               | `1`                 | Matrix-style glyph mix; `0` uses the code-point range     |
| `CHARS_LO` / `CHARS_HI`                       | `0x2800` / `0x28FF` | range used when `MATRIX_GLYPHS=0`                         |
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
