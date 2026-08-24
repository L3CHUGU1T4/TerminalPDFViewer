#include "tui.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

void gotoxy(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

std::string fit(const std::string &s, int w) {
    if (w <= 0) return "";
    if ((int)s.size() <= w) return s + std::string(w - s.size(), ' ');
    return s.substr(0, w - 1) + "…";
}

std::string short_path(const std::string &path) {
    const char *home = getenv("HOME");
    if (!home) return path;
    size_t hlen = strlen(home);
    if (path.substr(0, hlen) == home)
        return "~" + path.substr(hlen);
    return path;
}

int draw_box(int row, int col, int w, int h,
             const char *title, const char *title_col) {
    // ── Top border ──────────────────────────────────────────────────────────
    gotoxy(row, col);
    printf("%s%s╭─", C::panel, C::dim);
    if (title) {
        printf(" %s%s%s%s%s%s ─",
               title_col ? title_col : C::blue,
               C::bold, title, C::reset, C::panel, C::dim);
        int filled = 5 + (int)strlen(title);
        for (int i = filled; i < w - 2; i++) printf("─");
    } else {
        for (int i = 2; i < w - 1; i++) printf("─");
    }
    printf("╮%s", C::reset);

    // ── Content rows ────────────────────────────────────────────────────────
    for (int r = 1; r < h - 1; r++) {
        gotoxy(row + r, col);
        printf("%s%s│%s", C::panel, C::dim, C::reset);
        printf("%s", C::panel);
        for (int i = 0; i < w - 2; i++) printf(" ");
        printf("%s%s│%s", C::panel, C::dim, C::reset);
    }

    // ── Bottom border ───────────────────────────────────────────────────────
    gotoxy(row + h - 1, col);
    printf("%s%s╰", C::panel, C::dim);
    for (int i = 0; i < w - 2; i++) printf("─");
    printf("╯%s", C::reset);

    return row + 1;
}
