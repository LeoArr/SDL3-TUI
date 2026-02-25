#ifndef TUI_H
#define TUI_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdbool.h>
#include <stdint.h>

/* ── 16-color terminal palette ─────────────────────────── */

enum {
    TUI_BLACK, TUI_RED, TUI_GREEN, TUI_YELLOW,
    TUI_BLUE, TUI_MAGENTA, TUI_CYAN, TUI_WHITE,
    TUI_BRIGHT_BLACK, TUI_BRIGHT_RED, TUI_BRIGHT_GREEN, TUI_BRIGHT_YELLOW,
    TUI_BRIGHT_BLUE, TUI_BRIGHT_MAGENTA, TUI_BRIGHT_CYAN, TUI_BRIGHT_WHITE,
    TUI_PALETTE_SIZE
};

/* ── Cell ──────────────────────────────────────────────── */

typedef struct { char ch; uint8_t fg, bg; } TUI_Cell;

/* ── Context ───────────────────────────────────────────── */

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    TTF_Font     *font;
    SDL_Texture  *atlas;
    int           cell_w, cell_h;
    int           scale;
    int           cols, rows;
    TUI_Cell     *cells;
    SDL_Color     palette[TUI_PALETTE_SIZE];
    bool          running;
    uint64_t      blink_ms;
    bool          blink_on;
} TUI;

/* ── Lifecycle ─────────────────────────────────────────── */

bool tui_init   (TUI *t, const char *title, int win_w, int win_h,
                 const char *font_path, float font_size, int scale);
void tui_destroy(TUI *t);
void tui_set_scale(TUI *t, int scale);

/* ── Frame ─────────────────────────────────────────────── */

void tui_begin(TUI *t);
void tui_end  (TUI *t);

/* ── Drawing primitives ────────────────────────────────── */

void tui_clear    (TUI *t, uint8_t bg);
void tui_putc     (TUI *t, int x, int y, char ch, uint8_t fg, uint8_t bg);
void tui_puts     (TUI *t, int x, int y, const char *s, uint8_t fg, uint8_t bg);
int  tui_puts_wrap(TUI *t, int x, int y, int w, const char *s,
                   uint8_t fg, uint8_t bg);
void tui_hline    (TUI *t, int x, int y, int w, char ch, uint8_t fg, uint8_t bg);
void tui_vline    (TUI *t, int x, int y, int h, char ch, uint8_t fg, uint8_t bg);
void tui_fill     (TUI *t, int x, int y, int w, int h, char ch,
                   uint8_t fg, uint8_t bg);
void tui_box      (TUI *t, int x, int y, int w, int h, uint8_t fg, uint8_t bg);

/* ── Menu ──────────────────────────────────────────────── */

typedef struct {
    int  selected;
    int  confirmed;
    bool escaped;
} TUI_MenuState;

void tui_menu_init  (TUI_MenuState *s);
void tui_draw_menu_h(TUI *t, int x, int y,
                     const char **items, int count, const TUI_MenuState *s,
                     bool focused,
                     uint8_t fg, uint8_t bg, uint8_t sel_fg, uint8_t sel_bg);
void tui_draw_menu_v(TUI *t, int x, int y, int w,
                     const char **items, int count, const TUI_MenuState *s,
                     bool focused,
                     uint8_t fg, uint8_t bg, uint8_t sel_fg, uint8_t sel_bg);
bool tui_menu_handle(TUI_MenuState *s, const SDL_Event *e,
                     int count, bool horizontal);

/* ── Table ─────────────────────────────────────────────── */

#define TUI_TABLE_MAX_COLS 16

int tui_draw_table(TUI *t, int x, int y,
                   int col_count, int row_count,
                   const char **headers, const char **data,
                   const int *col_widths,
                   uint8_t fg, uint8_t bg,
                   uint8_t hdr_fg, uint8_t hdr_bg);

/* ── Text input field ──────────────────────────────────── */

#define TUI_INPUT_MAX 256

typedef struct {
    char text[TUI_INPUT_MAX];
    int  cursor, scroll, max_len;
} TUI_InputState;

void tui_input_init  (TUI_InputState *s, int max_len);
void tui_draw_input  (TUI *t, int x, int y, int w, TUI_InputState *s,
                      bool focused, uint8_t fg, uint8_t bg,
                      uint8_t cur_fg, uint8_t cur_bg);
bool tui_input_handle(TUI_InputState *s, const SDL_Event *e);
void tui_text_input_start(TUI *t);
void tui_text_input_stop (TUI *t);

/* ── Modal dialog ──────────────────────────────────────── */

typedef struct {
    int  selected;
    int  result;
    bool enforce;
    bool active;
} TUI_ModalState;

void tui_modal_open  (TUI_ModalState *s, bool enforce);
void tui_draw_modal  (TUI *t, const char *title, const char *msg,
                      const char **options, int count, TUI_ModalState *s,
                      uint8_t fg, uint8_t bg,
                      uint8_t sel_fg, uint8_t sel_bg);
bool tui_modal_handle(TUI_ModalState *s, const SDL_Event *e, int count);

/* ── Legend bar ─────────────────────────────────────────── */

typedef struct { const char *key; const char *desc; } TUI_LegendItem;

void tui_draw_legend(TUI *t, const TUI_LegendItem *items, int count,
                     uint8_t key_fg, uint8_t key_bg,
                     uint8_t desc_fg, uint8_t desc_bg);

#endif /* TUI_H */
