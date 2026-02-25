# SDL3 TUI-LIKE FRAMEWORK

Asked Claude 4.6 Opus to scetch out a TUI-like framework using SDL3 as a PoC for a project.
It gave me something interesting so I gave it some pointers and it took about five minutes, ~$15 worth of tokens and gave me this.

I am quite pleased with the results so having tweaked some minor things it might as well be shared here.

# SDL3 Character-Grid TUI

A lightweight terminal user interface framework rendered entirely as ASCII characters on a monospace cell grid, built from scratch with **C11** and **SDL3 + SDL3_ttf**.

Everything you see — boxes, menus, tables, input fields, modals — is drawn as characters, like a retro terminal emulator.


## Features

- **Character grid rendering** — all UI composed of ASCII glyphs via a prebuilt font atlas
- **16-color VGA palette** — classic terminal aesthetic
- **Drawing primitives** — `putc`, `puts`, `hline`, `vline`, `box`, `fill`, word-wrapping text
- **Horizontal & vertical menus** — arrow-key navigation, blinking focus indicator
- **Text input fields** — cursor movement, insert/delete, scrolling, blinking caret
- **Tables** — auto-sized or fixed-width columns with ASCII borders
- **Modal dialogs** — Yes/No prompts with optional forced choice (no Escape to cancel)
- **Terminal emulator** — scrollable command prompt with built-in demo commands
- **Legend bar** — context-sensitive key hints at the bottom of the screen
- **Integer zoom** — `+`/`-` keys scale the grid with nearest-neighbor filtering (pixel-perfect)
- **Responsive layout** — grid dimensions adapt dynamically to window size
- **Explicit focus model** — application code controls which widget receives input

## Files

| File | Description |
|---|---|
| `tui.h` | Public API — structs, enums, all function declarations |
| `tui.c` | Implementation — atlas, grid, drawing, widgets |
| `main.c` | Demo application with four tabs (General, Table, Terminal, About) |

## Dependencies

- [SDL3](https://github.com/libsdl-org/SDL) — windowing, rendering, input
- [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf) — font loading and glyph rasterization
- A monospace TTF font (e.g. `DejaVuSansMono.ttf`) placed alongside the binary

## Build

```bash
cc -std=c11 -o tui_demo main.c tui.c \
   $(pkg-config --cflags --libs sdl3 sdl3-ttf)
```

## Font

The font is (Good Old DOS)[https://www.dafont.com/good-old-dos.font] provided as Public Domain.
