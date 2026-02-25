#include "tui.h"
#include <SDL3/SDL_main.h>
#include <string.h>
#include <stdio.h>

/* ── Terminal emulator state ───────────────────────────── */

#define TERM_MAX_LINES 200
#define TERM_LINE_MAX  256

typedef struct {
    char    lines[TERM_MAX_LINES][TERM_LINE_MAX];
    uint8_t colors[TERM_MAX_LINES];
    int     count;
    int     scroll;
    TUI_InputState input;
} TermState;

static void term_init(TermState *ts)
{
    memset(ts, 0, sizeof *ts);
    tui_input_init(&ts->input, TERM_LINE_MAX - 1);
}

static void term_print(TermState *ts, const char *msg, uint8_t fg)
{
    if (ts->count >= TERM_MAX_LINES) {
        memmove(ts->lines[0], ts->lines[1],
                (size_t)(TERM_MAX_LINES - 1) * TERM_LINE_MAX);
        memmove(ts->colors, ts->colors + 1, TERM_MAX_LINES - 1);
        ts->count = TERM_MAX_LINES - 1;
    }
    snprintf(ts->lines[ts->count], TERM_LINE_MAX, "%s", msg);
    ts->colors[ts->count] = fg;
    ts->count++;
}

static void term_exec(TermState *ts)
{
    char *cmd = ts->input.text;

    if (strcmp(cmd, "clear") == 0) {
        ts->count  = 0;
        ts->scroll = 0;
    } else {
        char prompt[TERM_LINE_MAX];
        snprintf(prompt, sizeof prompt, "> %s", cmd);
        term_print(ts, prompt, TUI_GREEN);

        if (strcmp(cmd, "help") == 0) {
            term_print(ts, "Commands:", TUI_CYAN);
            term_print(ts, "  help    - Show this help", TUI_CYAN);
            term_print(ts, "  echo    - Echo text  (echo <msg>)", TUI_CYAN);
            term_print(ts, "  clear   - Clear terminal", TUI_CYAN);
            term_print(ts, "  time    - Show SDL ticks", TUI_CYAN);
            term_print(ts, "  hello   - Greeting", TUI_CYAN);
            term_print(ts, "  colors  - Show palette", TUI_CYAN);
            term_print(ts, "  version - Version info", TUI_CYAN);
        } else if (strncmp(cmd, "echo ", 5) == 0) {
            term_print(ts, cmd + 5, TUI_WHITE);
        } else if (strcmp(cmd, "hello") == 0) {
            term_print(ts, "Hello, World!", TUI_YELLOW);
        } else if (strcmp(cmd, "time") == 0) {
            char buf[64];
            snprintf(buf, sizeof buf, "Ticks: %llu",
                     (unsigned long long)SDL_GetTicks());
            term_print(ts, buf, TUI_WHITE);
        } else if (strcmp(cmd, "colors") == 0) {
            for (int i = 0; i < TUI_PALETTE_SIZE; i++) {
                char buf[32];
                snprintf(buf, sizeof buf, "  Color %2d ########", i);
                term_print(ts, buf, (uint8_t)i);
            }
        } else if (strcmp(cmd, "version") == 0) {
            term_print(ts, "TUI Demo v1.0", TUI_BRIGHT_MAGENTA);
        } else if (cmd[0] != '\0') {
            char buf[TERM_LINE_MAX];
            snprintf(buf, sizeof buf, "Unknown command: %s", cmd);
            term_print(ts, buf, TUI_BRIGHT_RED);
        }
        ts->scroll = 0;
    }

    ts->input.text[0] = '\0';
    ts->input.cursor   = 0;
    ts->input.scroll   = 0;
}

static void term_draw(TUI *t, TermState *ts,
                      int x, int y, int w, int h, bool focused)
{
    if (h < 6 || w < 10) return;

    tui_fill(t, x, y, w, h, ' ', TUI_WHITE, TUI_BLACK);
    tui_box (t, x, y, w, h, TUI_BRIGHT_BLACK, TUI_BLACK);

    /* title */
    const char *title = " Terminal ";
    int tl = (int)strlen(title);
    tui_puts(t, x + (w - tl) / 2, y, title, TUI_BRIGHT_WHITE, TUI_BLACK);

    /* layout: top border | output area | separator | input | bottom border */
    int vis = h - 4;

    int max_scroll = ts->count > vis ? ts->count - vis : 0;
    if (ts->scroll > max_scroll) ts->scroll = max_scroll;
    if (ts->scroll < 0) ts->scroll = 0;

    int first = ts->count - vis - ts->scroll;
    if (first < 0) first = 0;

    for (int i = 0; i < vis; i++) {
        int li = first + i;
        if (li >= 0 && li < ts->count) {
            int max_chars = w - 2;
            for (int c = 0; c < max_chars && ts->lines[li][c]; c++)
                tui_putc(t, x + 1 + c, y + 1 + i,
                         ts->lines[li][c], ts->colors[li], TUI_BLACK);
        }
    }

    /* scroll indicators */
    if (ts->scroll < max_scroll && max_scroll > 0)
        tui_putc(t, x + w - 2, y + 1, '^', TUI_YELLOW, TUI_BLACK);
    if (ts->scroll > 0)
        tui_putc(t, x + w - 2, y + h - 4, 'v', TUI_YELLOW, TUI_BLACK);

    /* separator and prompt */
    tui_hline(t, x + 1, y + h - 3, w - 2, '-', TUI_BRIGHT_BLACK, TUI_BLACK);
    tui_puts (t, x + 1, y + h - 2, "> ", TUI_GREEN, TUI_BLACK);

    int iw = w - 5;
    if (iw > 2)
        tui_draw_input(t, x + 3, y + h - 2, iw, &ts->input,
                       focused, TUI_WHITE, TUI_BLACK,
                       TUI_BLACK, TUI_WHITE);
}

/* ── Helpers ───────────────────────────────────────────── */

static void sync_text_input(TUI *t, int tab, int field)
{
    if ((tab == 0 && field < 2) || tab == 2)
        tui_text_input_start(t);
    else
        tui_text_input_stop(t);
}

/* ── Main ──────────────────────────────────────────────── */

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    TUI t;
    if (!tui_init(&t, "TUI Demo", 900, 560,
                  "Good Old DOS.ttf", 32.0f, 1)) {
        SDL_Log("Init failed: %s", SDL_GetError());
        return 1;
    }

    /* ── widget state ──────────────────────────────────── */
    const char *tabs[] = {"General", "Table", "Terminal", "About"};
    enum { TAB_GENERAL, TAB_TABLE, TAB_TERMINAL, TAB_ABOUT, TAB_COUNT };
    TUI_MenuState tab_menu;
    tui_menu_init(&tab_menu);

    TUI_InputState inp_name, inp_email;
    tui_input_init(&inp_name,  64);
    tui_input_init(&inp_email, 64);

    const char *actions[] = {"Save", "Load", "Reset"};
    TUI_MenuState act_menu;
    tui_menu_init(&act_menu);

    TUI_ModalState modal = {0};

    const char *th[] = {"Name", "Age", "City"};
    const char *td[] = {
        "Alice",   "30", "New York",
        "Bob",     "25", "Los Angeles",
        "Charlie", "35", "Chicago",
        "Diana",   "28", "Houston",
    };

    TermState term;
    term_init(&term);
    term_print(&term, "Welcome to TUI Terminal!", TUI_BRIGHT_CYAN);
    term_print(&term, "Type 'help' for a list of commands.", TUI_CYAN);

    bool on_tabs = true;
    int  field   = 0; /* sub-focus inside General tab */

    /* ── main loop ─────────────────────────────────────── */
    while (t.running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                t.running = false;
                break;
            }

            /* ── modal captures everything ─────────────── */
            if (modal.active) {
                tui_modal_handle(&modal, &e, 2);
                continue;
            }

            /* ── zoom (+/-) when not in a text field ───── */
            bool typing = !on_tabs
                && ((tab_menu.selected == TAB_GENERAL && field < 2)
                    || tab_menu.selected == TAB_TERMINAL);

            if (!typing && e.type == SDL_EVENT_KEY_DOWN) {
                if (e.key.key == SDLK_EQUALS
                    || e.key.key == SDLK_KP_PLUS) {
                    tui_set_scale(&t, t.scale + 1);
                    continue;
                }
                if (e.key.key == SDLK_MINUS
                    || e.key.key == SDLK_KP_MINUS) {
                    tui_set_scale(&t, t.scale > 1 ? t.scale - 1 : 1);
                    continue;
                }
            }

            /* ── tab bar focused ───────────────────────── */
            if (on_tabs) {
                tui_menu_handle(&tab_menu, &e, TAB_COUNT, true);
                if (tab_menu.confirmed >= 0) {
                    on_tabs = false;
                    field   = 0;
                    sync_text_input(&t, tab_menu.selected, field);
                }
                continue;
            }

            /* ── content focused ───────────────────────── */
            /* Escape always returns to tabs */
            if (e.type == SDL_EVENT_KEY_DOWN
                && e.key.key == SDLK_ESCAPE) {
                on_tabs = true;
                tui_text_input_stop(&t);
                continue;
            }

            switch (tab_menu.selected) {

            /* ── General tab ───────────────────────────── */
            case TAB_GENERAL:
                if (field == 0) {
                    if (!tui_input_handle(&inp_name, &e)
                        && e.type == SDL_EVENT_KEY_DOWN
                        && e.key.key == SDLK_TAB) {
                        field = (e.key.mod & SDL_KMOD_SHIFT) ? 2 : 1;
                        sync_text_input(&t, TAB_GENERAL, field);
                    }
                } else if (field == 1) {
                    if (!tui_input_handle(&inp_email, &e)
                        && e.type == SDL_EVENT_KEY_DOWN
                        && e.key.key == SDLK_TAB) {
                        field = (e.key.mod & SDL_KMOD_SHIFT) ? 0 : 2;
                        sync_text_input(&t, TAB_GENERAL, field);
                    }
                } else { /* field == 2: action menu */
                    if (e.type == SDL_EVENT_KEY_DOWN
                        && e.key.key == SDLK_TAB) {
                        field = (e.key.mod & SDL_KMOD_SHIFT) ? 1 : 0;
                        sync_text_input(&t, TAB_GENERAL, field);
                    } else {
                        tui_menu_handle(&act_menu, &e, 3, false);
                        if (act_menu.confirmed >= 0)
                            tui_modal_open(&modal, true);
                    }
                }
                break;

            /* ── Terminal tab ──────────────────────────── */
            case TAB_TERMINAL:
                if (e.type == SDL_EVENT_KEY_DOWN) {
                    if (e.key.key == SDLK_RETURN
                        || e.key.key == SDLK_KP_ENTER) {
                        if (term.input.text[0] != '\0')
                            term_exec(&term);
                        break;
                    }
                    if (e.key.key == SDLK_PAGEUP) {
                        term.scroll += 5;
                        break;
                    }
                    if (e.key.key == SDLK_PAGEDOWN) {
                        term.scroll -= 5;
                        if (term.scroll < 0) term.scroll = 0;
                        break;
                    }
                }
                tui_input_handle(&term.input, &e);
                break;

            /* ── Table / About – display only ──────────── */
            default:
                break;
            }
        }

        /* ── draw ──────────────────────────────────────── */
        tui_begin(&t);

        /* title bar */
        tui_fill(&t, 0, 0, t.cols, 1, ' ',
                 TUI_BRIGHT_WHITE, TUI_BLUE);
        {
            char title[64];
            snprintf(title, sizeof title, "TUI Demo  (scale %d)", t.scale);
            tui_puts(&t, 2, 0, title, TUI_BRIGHT_WHITE, TUI_BLUE);
        }

        /* tab bar + separator */
        tui_draw_menu_h(&t, 2, 2, tabs, TAB_COUNT, &tab_menu,
                        on_tabs,
                        TUI_WHITE, TUI_BLACK,
                        TUI_BRIGHT_WHITE, TUI_BLUE);
        tui_hline(&t, 0, 3, t.cols, '-', TUI_BRIGHT_BLACK, TUI_BLACK);

        int cy = 5;

        switch (tab_menu.selected) {

        case TAB_GENERAL: {
            tui_puts(&t, 2, cy, "Name:", TUI_WHITE, TUI_BLACK);
            tui_draw_input(&t, 10, cy, 30, &inp_name,
                           !on_tabs && field == 0,
                           TUI_WHITE, TUI_BLACK,
                           TUI_BLACK, TUI_WHITE);
            cy += 2;

            tui_puts(&t, 2, cy, "Email:", TUI_WHITE, TUI_BLACK);
            tui_draw_input(&t, 10, cy, 30, &inp_email,
                           !on_tabs && field == 1,
                           TUI_WHITE, TUI_BLACK,
                           TUI_BLACK, TUI_WHITE);
            cy += 2;

            tui_puts(&t, 2, cy++, "Action:", TUI_WHITE, TUI_BLACK);
            tui_draw_menu_v(&t, 2, cy, 20, actions, 3, &act_menu,
                            !on_tabs && field == 2,
                            TUI_WHITE, TUI_BLACK,
                            TUI_BRIGHT_WHITE, TUI_BLUE);
            break;
        }

        case TAB_TABLE:
            tui_draw_table(&t, 2, cy, 3, 4, th, td, NULL,
                           TUI_WHITE, TUI_BLACK,
                           TUI_BRIGHT_WHITE, TUI_BLUE);
            break;

        case TAB_TERMINAL: {
            int tw = t.cols - 2;
            int th2 = t.rows - cy - 2; /* leave room for legend */
            if (th2 < 6) th2 = 6;
            term_draw(&t, &term, 1, cy, tw, th2, !on_tabs);
            break;
        }

        case TAB_ABOUT:
            tui_box(&t, 1, cy - 1, t.cols - 2, 10,
                    TUI_BRIGHT_BLACK, TUI_BLACK);
            tui_puts_wrap(&t, 3, cy, t.cols - 6,
                "This is a lightweight character-grid TUI framework "
                "for SDL3. Everything is rendered as characters on a "
                "cell grid using a monospace font.\n\n"
                "Features: text wrapping, ASCII box drawing, "
                "horizontal and vertical menus, tables, input "
                "fields, modal dialogs, and a terminal emulator.\n\n"
                "All navigation is keyboard-driven.",
                TUI_WHITE, TUI_BLACK);
            break;
        }

        /* modal overlay */
        const char *mopts[] = {"Yes", "No"};
        tui_draw_modal(&t, "Confirm", "Execute this action?",
                       mopts, 2, &modal,
                       TUI_WHITE, TUI_BRIGHT_BLACK,
                       TUI_BRIGHT_WHITE, TUI_BLUE);

        /* context-sensitive legend bar */
        {
            uint8_t kf = TUI_BRIGHT_WHITE, kb = TUI_BLUE;
            uint8_t df = TUI_WHITE, db = TUI_BRIGHT_BLACK;

            if (modal.active) {
                if (modal.enforce) {
                    TUI_LegendItem l[] = {
                        {"</>", "Switch"}, {"Enter", "Confirm"}};
                    tui_draw_legend(&t, l, 2, kf, kb, df, db);
                } else {
                    TUI_LegendItem l[] = {
                        {"</>", "Switch"}, {"Enter", "Confirm"},
                        {"Esc", "Cancel"}};
                    tui_draw_legend(&t, l, 3, kf, kb, df, db);
                }
            } else if (on_tabs) {
                TUI_LegendItem l[] = {
                    {"</>", "Tab"}, {"Enter", "Open"},
                    {"+/-", "Zoom"}};
                tui_draw_legend(&t, l, 3, kf, kb, df, db);
            } else {
                switch (tab_menu.selected) {
                case TAB_GENERAL: {
                    TUI_LegendItem l[] = {
                        {"Tab", "Next"}, {"S-Tab", "Prev"},
                        {"Enter", "Select"}, {"Esc", "Back"}};
                    tui_draw_legend(&t, l, 4, kf, kb, df, db);
                    break;
                }
                case TAB_TERMINAL: {
                    TUI_LegendItem l[] = {
                        {"Enter", "Run"}, {"PgUp/Dn", "Scroll"},
                        {"Esc", "Back"}};
                    tui_draw_legend(&t, l, 3, kf, kb, df, db);
                    break;
                }
                default: {
                    TUI_LegendItem l[] = {
                        {"Esc", "Back"}, {"+/-", "Zoom"}};
                    tui_draw_legend(&t, l, 2, kf, kb, df, db);
                    break;
                }
                }
            }
        }

        tui_end(&t);
    }

    tui_destroy(&t);
    return 0;
}

