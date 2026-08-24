#include "home.h"
#include "term.h"
#include "tui.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

static const char *LOGO[] = {
    "   ╔═╗ ╔╦╗ ╔═╗  ╦  ╦╦╔═╗╦ ╦   ",
    "   ╠═╝  ║║ ╠╣   ╚╗╔╝║║╣ ║║║   ",
    "   ╩    ╩╝ ╚═╝   ╚╝ ╩╚═╝╚╩╝   ",
};
static constexpr int LOGO_LINES = 3;

std::string run_home(std::vector<std::string> &history) {
    std::string input;
    int         sel = -1;   // -1 = input field active, 0+ = history item
    std::string error;

    for (;;) {
        if (g_resized) g_resized = 0;
        auto ts = get_term_size();

        const int W  = std::min(60, ts.cols - 4);
        const int cx = (ts.cols - W) / 2 + 1;

        const int LOGO_H  = LOGO_LINES + 4;  // 3 logo lines + 1 subtitle + 2 borders
        const int INPUT_H = 4;
        int nhist         = std::min((int)history.size(), 5);
        const int HIST_H  = nhist > 0 ? nhist + 3 : 0;
        int total_h       = LOGO_H + 1 + INPUT_H + (HIST_H > 0 ? 1 + HIST_H : 0) + 2;
        int start_row     = std::max(2, (ts.rows - total_h) / 2);

        // Fill background
        printf("\033[2J%s", C::bg);
        for (int r = 1; r <= ts.rows; r++) {
            gotoxy(r, 1);
            for (int c = 0; c < ts.cols; c++) printf(" ");
        }

        int r = start_row;

        // ── Logo box ──────────────────────────────────────────────────────
        draw_box(r, cx, W, LOGO_H);
        for (int i = 0; i < LOGO_LINES; i++) {
            gotoxy(r + 1 + i, cx + 1);
            int inner  = W - 2;
            int llen   = (int)strlen(LOGO[i]);
            int pad    = (inner - llen) / 2;
            printf("%s", C::panel);
            for (int j = 0; j < pad; j++) printf(" ");
            printf("%s%s%s%s%s", C::blue, C::bold, LOGO[i], C::reset, C::panel);
            for (int j = pad + llen; j < inner; j++) printf(" ");
            printf("%s", C::reset);
        }
        {
            gotoxy(r + 1 + LOGO_LINES, cx + 1);
            const char *sub  = "  terminal · pdf · reader";
            int slen = (int)strlen(sub);
            int pad  = (W - 2 - slen) / 2;
            printf("%s", C::panel);
            for (int j = 0; j < pad; j++) printf(" ");
            printf("%s%s%s", C::dim, sub, C::reset);
            printf("%s", C::panel);
            for (int j = pad + slen; j < W - 2; j++) printf(" ");
            printf("%s", C::reset);
        }
        r += LOGO_H + 1;

        // ── Open / input box ──────────────────────────────────────────────
        draw_box(r, cx, W, INPUT_H, "open", C::blue);
        {
            gotoxy(r + 1, cx + 1);
            bool active = (sel == -1);
            printf("%s  %s%s▸ ", C::panel, active ? C::blue : C::dim, active ? C::bold : "");
            printf("%s%s%s", C::reset, C::panel, active ? C::bright : C::dim);

            std::string disp = (input.empty() && !active) ? "type a path or select below" : input;
            int avail = W - 6;
            if ((int)disp.size() > avail) disp = disp.substr(disp.size() - avail);
            printf("%s", disp.c_str());
            if (active) printf("%s█%s", C::blue, C::reset);
            printf("%s", C::panel);
            int used = (int)disp.size() + (active ? 1 : 0);
            for (int j = used; j < avail; j++) printf(" ");
            printf("%s", C::reset);

            gotoxy(r + 2, cx + 1);
            printf("%s  ", C::panel);
            if (!error.empty())
                printf("%s%s%s", C::red, error.c_str(), C::reset);
            else
                printf("%s↵ to open   ↑↓ to select recent%s", C::dim, C::reset);
            printf("%s", C::panel);
            int hlen = error.empty() ? 35 : (int)error.size();
            for (int j = hlen; j < W - 4; j++) printf(" ");
            printf("%s", C::reset);
        }
        r += INPUT_H + 1;

        // ── Recent files box ──────────────────────────────────────────────
        if (HIST_H > 0) {
            draw_box(r, cx, W, HIST_H, "recent", C::purple);
            for (int i = 0; i < nhist; i++) {
                bool is_sel = (sel == i);
                gotoxy(r + 1 + i, cx + 1);
                printf("%s", is_sel ? C::sel : C::panel);
                printf("  %s%s ", is_sel ? C::blue : C::dim, is_sel ? "▶" : "○");

                std::string p    = short_path(history[i]);
                size_t      sl2  = p.rfind('/');
                std::string name = (sl2 != std::string::npos) ? p.substr(sl2 + 1) : p;
                std::string dir  = (sl2 != std::string::npos) ? p.substr(0, sl2 + 1) : "";

                int avail = W - 7;
                int dw    = std::min((int)dir.size(), avail / 3);
                int nw    = avail - dw - 1;

                printf("%s%s%s", C::dim, fit(dir, dw).c_str(), C::reset);
                printf("%s%s%s%s%s",
                       is_sel ? C::sel : C::panel,
                       is_sel ? C::bright : C::text,
                       is_sel ? C::bold : "",
                       fit(name, nw).c_str(), C::reset);
            }
            gotoxy(r + nhist + 1, cx + 1);
            printf("%s%s  ↵ open   ↑↓ navigate%s%s",
                   C::panel, C::dim, C::reset, C::panel);
            for (int j = 24; j < W - 2; j++) printf(" ");
            printf("%s", C::reset);
        }

        // ── Bottom hint bar ───────────────────────────────────────────────
        gotoxy(ts.rows, 1);
        printf("%s%s", C::bar, C::dim);
        const char *hint = "  ↵ open   ↑↓ navigate   ⌫ delete char   q quit";
        printf("%s", hint);
        for (int i = (int)strlen(hint); i < ts.cols; i++) printf(" ");
        printf("%s", C::reset);

        fflush(stdout);

        // ── Input handling ────────────────────────────────────────────────
        int raw = read_byte(-1);
        if (raw < 0) continue;
        int key = decode_key(raw);
        error.clear();

        if (sel == -1) {
            switch (key) {
                case KEY_ENTER:
                    if (!input.empty()) return input;
                    break;
                case KEY_BACKSPACE: case KEY_DEL:
                    if (!input.empty()) input.pop_back();
                    break;
                case KEY_UP: case KEY_DOWN:
                    if (!history.empty()) sel = 0;
                    break;
                case 'q':
                    if (input.empty()) return "";
                    input += 'q';
                    break;
                default:
                    if (raw >= 32 && raw < 127) input += char(raw);
            }
        } else {
            switch (key) {
                case KEY_ENTER:
                    return history[sel];
                case KEY_UP:
                    if (--sel < 0) sel = -1;
                    break;
                case KEY_DOWN:
                    if (sel < nhist - 1) sel++;
                    break;
                case KEY_ESC: case 'q':
                    return "";
                case KEY_BACKSPACE:
                    sel = -1;
                    break;
                default:
                    if (raw >= 32 && raw < 127) { sel = -1; input += char(raw); }
            }
        }
    }
}
