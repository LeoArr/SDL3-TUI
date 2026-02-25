#include "tui.h"
#include <SDL3/SDL_keycode.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Default VGA palette ───────────────────────────────── */

static const SDL_Color default_pal[TUI_PALETTE_SIZE] = {
    {  0,   0,   0, 255}, {170,   0,   0, 255},
    {  0, 170,   0, 255}, {170,  85,   0, 255},
    {  0,   0, 170, 255}, {170,   0, 170, 255},
    {  0, 170, 170, 255}, {170, 170, 170, 255},
    { 85,  85,  85, 255}, {255,  85,  85, 255},
    { 85, 255,  85, 255}, {255, 255,  85, 255},
    { 85,  85, 255, 255}, {255,  85, 255, 255},
    { 85, 255, 255, 255}, {255, 255, 255, 255},
};

/* ── Atlas (printable ASCII 32..126) ───────────────────── */

static bool create_atlas(TUI *t)
{
    SDL_Color white = {255, 255, 255, 255};
    char buf[2] = "M";

    SDL_Surface *ref = TTF_RenderText_Blended(t->font, buf, 0, white);
    if (!ref) return false;
    SDL_PixelFormat fmt = ref->format;
    SDL_DestroySurface(ref);

    int count = 95;
    SDL_Surface *atlas = SDL_CreateSurface(
        t->cell_w * count, t->cell_h, fmt);
    if (!atlas) return false;
    SDL_FillSurfaceRect(atlas, NULL,
                        SDL_MapSurfaceRGBA(atlas, 0, 0, 0, 0));

    for (int i = 0; i < count; i++) {
        buf[0] = (char)(32 + i);
        SDL_Surface *g = TTF_RenderText_Blended(t->font, buf, 0, white);
        if (g) {
            SDL_SetSurfaceBlendMode(g, SDL_BLENDMODE_NONE);
            SDL_Rect d = {i * t->cell_w, 0, g->w, g->h};
            SDL_BlitSurface(g, NULL, atlas, &d);
            SDL_DestroySurface(g);
        }
    }

    t->atlas = SDL_CreateTextureFromSurface(t->renderer, atlas);
    SDL_DestroySurface(atlas);
    if (!t->atlas) return false;
    SDL_SetTextureBlendMode(t->atlas, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(t->atlas, SDL_SCALEMODE_NEAREST);
    return true;
}

/* ── Grid resize ───────────────────────────────────────── */

static void resize_grid(TUI *t)
{
    int w, h;
    SDL_GetWindowSize(t->window, &w, &h);
    int nc = w / (t->cell_w * t->scale);
    int nr = h / (t->cell_h * t->scale);
    if (nc < 1) nc = 1;
    if (nr < 1) nr = 1;
    if (nc != t->cols || nr != t->rows) {
        t->cols = nc;
        t->rows = nr;
        free(t->cells);
        t->cells = calloc((size_t)(nc * nr), sizeof(TUI_Cell));
    }
}

/* ── Lifecycle ─────────────────────────────────────────── */

bool tui_init(TUI *t, const char *title, int win_w, int win_h,
              const char *font_path, float font_size, int scale)
{
    memset(t, 0, sizeof *t);
    t->scale = scale < 1 ? 1 : scale;
    memcpy(t->palette, default_pal, sizeof default_pal);

    if (!SDL_Init(SDL_INIT_VIDEO)) return false;
    if (!TTF_Init())               return false;

    t->font = TTF_OpenFont(font_path, font_size);
    if (!t->font) return false;

    TTF_GetStringSize(t->font, "M", 0, &t->cell_w, &t->cell_h);
    if (t->cell_w <= 0 || t->cell_h <= 0) return false;

    t->window = SDL_CreateWindow(title, win_w, win_h,
                                 SDL_WINDOW_RESIZABLE);
    if (!t->window) return false;

    t->renderer = SDL_CreateRenderer(t->window, NULL);
    if (!t->renderer) return false;
    SDL_SetRenderVSync(t->renderer, 1);


    if (!create_atlas(t)) return false;

    t->cols = win_w / (t->cell_w * t->scale);
    t->rows = win_h / (t->cell_h * t->scale);
    if (t->cols < 1) t->cols = 1;
    if (t->rows < 1) t->rows = 1;
    t->cells = calloc((size_t)(t->cols * t->rows), sizeof(TUI_Cell));

    t->blink_ms = SDL_GetTicks();
    t->blink_on = true;
    t->running  = true;
    return true;
}

void tui_destroy(TUI *t)
{
    free(t->cells);
    if (t->atlas)    SDL_DestroyTexture(t->atlas);
    if (t->font)     TTF_CloseFont(t->font);
    TTF_Quit();
    if (t->renderer) SDL_DestroyRenderer(t->renderer);
    if (t->window)   SDL_DestroyWindow(t->window);
    SDL_Quit();
}

void tui_set_scale(TUI *t, int scale)
{
    if (scale < 1) scale = 1;
    int w, h;
    SDL_GetWindowSize(t->window, &w, &h);
    int nc = w / (t->cell_w * scale);
    int nr = h / (t->cell_h * scale);
    if (nc < 20 || nr < 8) return;   /* reject if grid too small */
    t->scale = scale;
    resize_grid(t);
}

/* ── Frame ─────────────────────────────────────────────── */

void tui_begin(TUI *t)
{
    resize_grid(t);
    tui_clear(t, TUI_BLACK);

    uint64_t now = SDL_GetTicks();
    if (now - t->blink_ms >= 500) {
        t->blink_on = !t->blink_on;
        t->blink_ms = now;
    }
}

void tui_end(TUI *t)
{
    SDL_SetRenderDrawColor(t->renderer, 0, 0, 0, 255);
    SDL_RenderClear(t->renderer);

    int s  = t->scale;
    int cw = t->cell_w;
    int ch = t->cell_h;

    for (int r = 0; r < t->rows; r++) {
        for (int c = 0; c < t->cols; c++) {
            TUI_Cell *cell = &t->cells[r * t->cols + c];
            float px = (float)(c * cw * s);
            float py = (float)(r * ch * s);
            float pw = (float)(cw * s);
            float ph = (float)(ch * s);
            SDL_FRect dst = {px, py, pw, ph};

            SDL_Color bg = t->palette[cell->bg % TUI_PALETTE_SIZE];
            SDL_SetRenderDrawColor(t->renderer, bg.r, bg.g, bg.b, 255);
            SDL_RenderFillRect(t->renderer, &dst);

            unsigned char v = (unsigned char)cell->ch;
            if (v >= 32 && v <= 126) {
                SDL_FRect src = {(float)((v - 32) * cw), 0,
                                 (float)cw, (float)ch};
                SDL_Color fg = t->palette[cell->fg % TUI_PALETTE_SIZE];
                SDL_SetTextureColorMod(t->atlas, fg.r, fg.g, fg.b);
                SDL_RenderTexture(t->renderer, t->atlas, &src, &dst);
            }
        }
    }
    SDL_RenderPresent(t->renderer);
}

/* ── Drawing primitives ────────────────────────────────── */

void tui_clear(TUI *t, uint8_t bg)
{
    int n = t->cols * t->rows;
    for (int i = 0; i < n; i++)
        t->cells[i] = (TUI_Cell){' ', TUI_WHITE, bg};
}

void tui_putc(TUI *t, int x, int y, char ch, uint8_t fg, uint8_t bg)
{
    if (x < 0 || x >= t->cols || y < 0 || y >= t->rows) return;
    t->cells[y * t->cols + x] = (TUI_Cell){ch, fg, bg};
}

void tui_puts(TUI *t, int x, int y, const char *s, uint8_t fg, uint8_t bg)
{
    for (int i = 0; s[i]; i++)
        tui_putc(t, x + i, y, s[i], fg, bg);
}

int tui_puts_wrap(TUI *t, int x, int y, int w, const char *s,
                  uint8_t fg, uint8_t bg)
{
    if (w <= 0) return 0;
    int cx = 0, cy = 0, i = 0;
    while (s[i]) {
        if (s[i] == '\n') { cx = 0; cy++; i++; continue; }

        int wl = 0;
        while (s[i + wl] && s[i + wl] != ' ' && s[i + wl] != '\n') wl++;

        if (wl > w) {
            for (int j = 0; j < wl; j++) {
                if (cx >= w) { cx = 0; cy++; }
                tui_putc(t, x + cx, y + cy, s[i + j], fg, bg);
                cx++;
            }
        } else {
            if (cx > 0 && cx + wl > w) { cx = 0; cy++; }
            for (int j = 0; j < wl; j++) {
                tui_putc(t, x + cx, y + cy, s[i + j], fg, bg);
                cx++;
            }
        }
        i += wl;
        if (s[i] == ' ') {
            i++;
            cx++;
            if (cx >= w) { cx = 0; cy++; }
        }
    }
    return cy + 1;
}

void tui_hline(TUI *t, int x, int y, int w, char ch,
               uint8_t fg, uint8_t bg)
{
    for (int i = 0; i < w; i++) tui_putc(t, x + i, y, ch, fg, bg);
}

void tui_vline(TUI *t, int x, int y, int h, char ch,
               uint8_t fg, uint8_t bg)
{
    for (int i = 0; i < h; i++) tui_putc(t, x, y + i, ch, fg, bg);
}

void tui_fill(TUI *t, int x, int y, int w, int h, char ch,
              uint8_t fg, uint8_t bg)
{
    for (int r = 0; r < h; r++)
        for (int c = 0; c < w; c++)
            tui_putc(t, x + c, y + r, ch, fg, bg);
}

void tui_box(TUI *t, int x, int y, int w, int h, uint8_t fg, uint8_t bg)
{
    if (w < 2 || h < 2) return;
    tui_putc(t, x,       y,       '+', fg, bg);
    tui_putc(t, x + w-1, y,       '+', fg, bg);
    tui_putc(t, x,       y + h-1, '+', fg, bg);
    tui_putc(t, x + w-1, y + h-1, '+', fg, bg);
    tui_hline(t, x + 1, y,       w - 2, '-', fg, bg);
    tui_hline(t, x + 1, y + h-1, w - 2, '-', fg, bg);
    tui_vline(t, x,       y + 1, h - 2, '|', fg, bg);
    tui_vline(t, x + w-1, y + 1, h - 2, '|', fg, bg);
}

/* ── Menu ──────────────────────────────────────────────── */

void tui_menu_init(TUI_MenuState *s)
{
    s->selected  = 0;
    s->confirmed = -1;
    s->escaped   = false;
}

void tui_draw_menu_h(TUI *t, int x, int y,
                     const char **items, int count, const TUI_MenuState *s,
                     bool focused,
                     uint8_t fg, uint8_t bg, uint8_t sf, uint8_t sb)
{
    int cx = x;
    for (int i = 0; i < count; i++) {
        bool sel = (i == s->selected);
        bool highlight = sel && (!focused || t->blink_on);
        uint8_t f = highlight ? sf : fg;
        uint8_t b = highlight ? sb : bg;

        tui_putc(t, cx++, y, ' ', f, b);
        tui_puts(t, cx, y, items[i], f, b);
        cx += (int)strlen(items[i]);
        tui_putc(t, cx++, y, ' ', f, b);
        if (i < count - 1)
            tui_putc(t, cx++, y, ' ', fg, bg);
    }
}

void tui_draw_menu_v(TUI *t, int x, int y, int w,
                     const char **items, int count, const TUI_MenuState *s,
                     bool focused,
                     uint8_t fg, uint8_t bg, uint8_t sf, uint8_t sb)
{
    for (int i = 0; i < count; i++) {
        bool sel = (i == s->selected);
        uint8_t f = sel ? sf : fg;
        uint8_t b = sel ? sb : bg;
        tui_fill(t, x, y + i, w, 1, ' ', f, b);

        char marker = ' ';
        if (sel) {
            if (focused)
                marker = t->blink_on ? '>' : ' ';
            else
                marker = '>';
        }
        tui_putc(t, x, y + i, marker, f, b);
        tui_puts(t, x + 2, y + i, items[i], f, b);
    }
}

bool tui_menu_handle(TUI_MenuState *s, const SDL_Event *e,
                     int count, bool horizontal)
{
    s->confirmed = -1;
    s->escaped   = false;
    if (e->type != SDL_EVENT_KEY_DOWN) return false;

    bool prev = horizontal ? (e->key.mod & SDL_KMOD_SHIFT) && e->key.key == SDLK_TAB : e->key.key == SDLK_UP;
    bool next = horizontal ? e->key.key == SDLK_TAB : e->key.key == SDLK_DOWN;

    if (prev) {
        if (s->selected > 0) s->selected--;
        return true;
    }
    if (next) {
        if (s->selected < count - 1) s->selected++;
        return true;
    }
    if (e->key.key == SDLK_RETURN || e->key.key == SDLK_KP_ENTER) {
        s->confirmed = s->selected;
        return true;
    }
    if (e->key.key == SDLK_ESCAPE) {
        s->escaped = true;
        return true;
    }
    return false;
}

/* ── Table ─────────────────────────────────────────────── */

static void tbl_sep(TUI *t, int x, int y, int cc, const int *w,
                    uint8_t fg, uint8_t bg)
{
    int cx = x;
    tui_putc(t, cx++, y, '+', fg, bg);
    for (int c = 0; c < cc; c++) {
        tui_hline(t, cx, y, w[c] + 2, '-', fg, bg);
        cx += w[c] + 2;
        tui_putc(t, cx++, y, '+', fg, bg);
    }
}

static void tbl_row(TUI *t, int x, int y, int cc, const int *w,
                    const char **cells, uint8_t fg, uint8_t bg,
                    uint8_t bfg)
{
    int cx = x;
    tui_putc(t, cx++, y, '|', bfg, bg);
    for (int c = 0; c < cc; c++) {
        tui_putc(t, cx++, y, ' ', fg, bg);
        int slen = (int)strlen(cells[c]);
        tui_puts(t, cx, y, cells[c], fg, bg);
        for (int p = slen; p < w[c]; p++)
            tui_putc(t, cx + p, y, ' ', fg, bg);
        cx += w[c];
        tui_putc(t, cx++, y, ' ', fg, bg);
        tui_putc(t, cx++, y, '|', bfg, bg);
    }
}

int tui_draw_table(TUI *t, int x, int y,
                   int col_count, int row_count,
                   const char **headers, const char **data,
                   const int *col_widths,
                   uint8_t fg, uint8_t bg,
                   uint8_t hdr_fg, uint8_t hdr_bg)
{
    int cc = col_count < TUI_TABLE_MAX_COLS ? col_count : TUI_TABLE_MAX_COLS;
    int widths[TUI_TABLE_MAX_COLS];

    for (int c = 0; c < cc; c++) {
        if (col_widths) { widths[c] = col_widths[c]; continue; }
        widths[c] = (int)strlen(headers[c]);
        for (int r = 0; r < row_count; r++) {
            int l = (int)strlen(data[r * cc + c]);
            if (l > widths[c]) widths[c] = l;
        }
    }

    int cy = y;
    tbl_sep(t, x, cy++, cc, widths, fg, bg);
    tbl_row(t, x, cy++, cc, widths, headers, hdr_fg, hdr_bg, fg);
    tbl_sep(t, x, cy++, cc, widths, fg, bg);
    for (int r = 0; r < row_count; r++)
        tbl_row(t, x, cy++, cc, widths, data + r * cc, fg, bg, fg);
    tbl_sep(t, x, cy++, cc, widths, fg, bg);
    return cy - y;
}

/* ── Input ─────────────────────────────────────────────── */

void tui_input_init(TUI_InputState *s, int max_len)
{
    memset(s, 0, sizeof *s);
    s->max_len = max_len < TUI_INPUT_MAX ? max_len : TUI_INPUT_MAX - 1;
}

void tui_draw_input(TUI *t, int x, int y, int w, TUI_InputState *s,
                    bool focused, uint8_t fg, uint8_t bg,
                    uint8_t cf, uint8_t cb)
{
    int inner = w - 2;
    if (inner < 1) return;

    if (s->cursor < s->scroll) s->scroll = s->cursor;
    if (s->cursor >= s->scroll + inner) s->scroll = s->cursor - inner + 1;
    if (s->scroll < 0) s->scroll = 0;

    /* brackets always visible with base colours */
    tui_putc(t, x, y, '[', fg, bg);
    tui_putc(t, x + w - 1, y, ']', fg, bg);

    int slen = (int)strlen(s->text);
    for (int i = 0; i < inner; i++) {
        int ti = s->scroll + i;
        char ch = (ti < slen) ? s->text[ti] : ' ';
        bool at_cur = focused && (ti == s->cursor) && t->blink_on;
        tui_putc(t, x + 1 + i, y, ch,
                 at_cur ? cf : fg,
                 at_cur ? cb : bg);
    }
}

bool tui_input_handle(TUI_InputState *s, const SDL_Event *e)
{
    if (e->type == SDL_EVENT_TEXT_INPUT) {
        int len = (int)strlen(s->text);
        int add = (int)strlen(e->text.text);
        if (len + add <= s->max_len) {
            memmove(s->text + s->cursor + add,
                    s->text + s->cursor,
                    (size_t)(len - s->cursor + 1));
            memcpy(s->text + s->cursor, e->text.text, (size_t)add);
            s->cursor += add;
        }
        return true;
    }
    if (e->type != SDL_EVENT_KEY_DOWN) return false;
    int len = (int)strlen(s->text);
    switch (e->key.key) {
    case SDLK_BACKSPACE:
        if (s->cursor > 0) {
            memmove(s->text + s->cursor - 1,
                    s->text + s->cursor,
                    (size_t)(len - s->cursor + 1));
            s->cursor--;
        }
        return true;
    case SDLK_DELETE:
        if (s->cursor < len)
            memmove(s->text + s->cursor,
                    s->text + s->cursor + 1,
                    (size_t)(len - s->cursor));
        return true;
    case SDLK_LEFT:  if (s->cursor > 0)   s->cursor--; return true;
    case SDLK_RIGHT: if (s->cursor < len) s->cursor++; return true;
    case SDLK_HOME:  s->cursor = 0;   return true;
    case SDLK_END:   s->cursor = len; return true;
    default: break;
    }
    return false;
}

void tui_text_input_start(TUI *t) { SDL_StartTextInput(t->window); }
void tui_text_input_stop (TUI *t) { SDL_StopTextInput(t->window);  }

/* ── Modal ─────────────────────────────────────────────── */

void tui_modal_open(TUI_ModalState *s, bool enforce)
{
    s->active   = true;
    s->selected = 0;
    s->result   = -1;
    s->enforce  = enforce;
}

void tui_draw_modal(TUI *t, const char *title, const char *msg,
                    const char **options, int count, TUI_ModalState *s,
                    uint8_t fg, uint8_t bg, uint8_t sf, uint8_t sb)
{
    if (!s->active) return;

    int tl = (int)strlen(title);
    int ml = (int)strlen(msg);
    int ol = 0;
    for (int i = 0; i < count; i++)
        ol += (int)strlen(options[i]) + 5;

    int iw = tl;
    if (ml > iw) iw = ml;
    if (ol > iw) iw = ol;
    iw += 4;
    int bw = iw + 2;
    int bh = 7;
    if (bw > t->cols - 2) bw = t->cols - 2;
    int bx = (t->cols - bw) / 2;
    int by = (t->rows - bh) / 2;

    tui_fill(t, bx, by, bw, bh, ' ', fg, bg);
    tui_box (t, bx, by, bw, bh, fg, bg);

    int tx = bx + (bw - tl - 2) / 2;
    tui_putc(t, tx, by, ' ', sf, bg);
    tui_puts(t, tx + 1, by, title, sf, bg);
    tui_putc(t, tx + 1 + tl, by, ' ', sf, bg);

    tui_puts(t, bx + 2, by + 2, msg, fg, bg);

    /* option buttons – solid colour, no blink */
    int ox = bx + 2;
    for (int i = 0; i < count; i++) {
        bool sel = (i == s->selected);
        char buf[80];
        snprintf(buf, sizeof buf, "[ %s ]", options[i]);
        int bl = (int)strlen(buf);
        tui_puts(t, ox, by + bh - 2, buf,
                 sel ? sf : fg,
                 sel ? sb : bg);
        ox += bl + 1;
    }
}

bool tui_modal_handle(TUI_ModalState *s, const SDL_Event *e, int count)
{
    if (!s->active || e->type != SDL_EVENT_KEY_DOWN) return false;
    switch (e->key.key) {
    case SDLK_LEFT:
        if (s->selected > 0) s->selected--;
        return true;
    case SDLK_RIGHT:
        if (s->selected < count - 1) s->selected++;
        return true;
    case SDLK_RETURN: case SDLK_KP_ENTER:
        s->result = s->selected;
        s->active = false;
        return true;
    case SDLK_ESCAPE:
        if (!s->enforce) {
            s->result = -1;
            s->active = false;
        }
        return true;
    default: break;
    }
    return false;
}

/* ── Legend ─────────────────────────────────────────────── */

void tui_draw_legend(TUI *t, const TUI_LegendItem *items, int count,
                     uint8_t kf, uint8_t kb, uint8_t df, uint8_t db)
{
    int y = t->rows - 1;
    tui_fill(t, 0, y, t->cols, 1, ' ', df, db);
    int cx = 1;
    for (int i = 0; i < count && cx < t->cols; i++) {
        int kl = (int)strlen(items[i].key);
        int dl = (int)strlen(items[i].desc);
        tui_puts(t, cx, y, items[i].key, kf, kb);
        cx += kl;
        tui_putc(t, cx++, y, ' ', df, db);
        tui_puts(t, cx, y, items[i].desc, df, db);
        cx += dl;
        if (i < count - 1) {
            tui_puts(t, cx, y, " | ", df, db);
            cx += 3;
        }
    }
}

