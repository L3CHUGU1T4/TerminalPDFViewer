#include "viewer.h"
#include "history.h"
#include "render.h"
#include "term.h"
#include "tui.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include <poppler-page-renderer.h>

// ─── Header bar ───────────────────────────────────────────────────────────────
static void draw_header(int cols, const std::string &filepath) {
    gotoxy(1, 1);
    printf("%s%s  ◈  pdfview  %s", C::bar, C::purple, C::reset);
    printf("%s%s│%s", C::bar, C::dim, C::reset);

    std::string name = short_path(filepath);
    size_t sl = name.rfind('/');
    std::string dir  = (sl != std::string::npos) ? name.substr(0, sl + 1) : "";
    std::string base = (sl != std::string::npos) ? name.substr(sl + 1) : name;

    printf("%s  %s%s%s%s%s",   C::bar, C::dim, dir.c_str(), C::reset, C::bar, C::reset);
    printf("%s%s%s%s%s",       C::bar, C::bright, C::bold, base.c_str(), C::reset);

    int used = 17 + (int)name.size() + 2;
    printf("%s", C::bar);
    for (int i = used; i < cols; i++) printf(" ");
    printf("%s", C::reset);
}

// ─── Status bar ───────────────────────────────────────────────────────────────
static void draw_status(int row, int cols, int page, int total,
                        double zoom, bool spread,
                        bool jump_mode, const std::string &jump_input) {
    gotoxy(row, 1);
    printf("%s", C::bar);

    if (jump_mode) {
        printf("%s  :%s%s%s%s_", C::cyan, C::reset, C::bar, C::bright, jump_input.c_str());
        int used = 3 + (int)jump_input.size() + 1;
        for (int i = used; i < cols; i++) printf(" ");
        printf("%s", C::reset);
        return;
    }

    printf("%s%s  %s  %s", C::blue, C::bold, spread ? "◫" : "◻", C::reset);
    printf("%s%s", C::bar, C::text);
    if (spread && page < total)
        printf(" %d–%d / %d ", page, std::min(page + 1, total), total);
    else
        printf(" %d / %d ", page, total);

    printf("%s%s │ %s%.0f%% │%s", C::dim, C::bar, C::text, zoom * 100.0, C::reset);

    const char *hints = "  s spread   t toc   + - zoom   : jump   ? help   q quit  ";
    int hlen     = (int)strlen(hints);
    int left_est = 4 + (spread ? 10 : 8) + (int)std::to_string(total).size() * 2 + 12;
    int pad      = cols - left_est - hlen;
    printf("%s", C::bar);
    for (int i = 0; i < pad && i < cols; i++) printf(" ");
    printf("%s%s%s", C::dim, hints, C::reset);
}

// ─── Help overlay ─────────────────────────────────────────────────────────────
static void draw_help(TermSize ts) {
    const int W = 52, H = 22;
    int r = std::max(1, (ts.rows - H) / 2);
    int c = std::max(1, (ts.cols - W) / 2);

    for (int row = r + 1; row < r + H + 1; row++) {
        gotoxy(row, c + 2);
        printf("\033[48;2;10;10;18m");
        for (int i = 0; i < W; i++) printf(" ");
        printf("%s", C::reset);
    }
    draw_box(r, c, W, H, "keyboard shortcuts", C::cyan);

    static const char *keys[][2] = {
        {"←  →  h  l",  "Previous / next page"},
        {"Space",        "Next page"},
        {"g  G",         "First / last page"},
        {":  [num] ↵",  "Jump to page"},
        {"",             ""},
        {"+  =",         "Zoom in"},
        {"-",            "Zoom out"},
        {"0",            "Reset zoom"},
        {"",             ""},
        {"s",            "Toggle spread (two-page view)"},
        {"t",            "Toggle table of contents"},
        {"?",            "Toggle this help"},
        {"",             ""},
        {"q  Q  Ctrl-C", "Quit"},
    };

    int row = r + 2;
    for (auto &k : keys) {
        gotoxy(row++, c + 1);
        if (strlen(k[0]) == 0) {
            printf("%s%s│%s%s", C::panel, C::dim, C::reset, C::panel);
            for (int i = 0; i < W - 2; i++) printf(" ");
            printf("%s%s│%s", C::panel, C::dim, C::reset);
            continue;
        }
        printf("%s%s│%s  %s%-18s%s  %s%-25s  %s%s│%s",
               C::panel, C::dim, C::reset,
               C::cyan,  k[0],   C::reset,
               C::panel, k[1],
               C::panel, C::dim, C::reset);
    }
    gotoxy(r + H - 2, c + 1);
    printf("%s%s│%s%s  press %s?%s or %sESC%s to close",
           C::panel, C::dim, C::reset, C::panel,
           C::blue, C::reset, C::panel, C::dim);
    printf("%s%s", C::panel, C::dim);
    for (int i = 25; i < W - 2; i++) printf(" ");
    printf("│%s", C::reset);
    fflush(stdout);
}

// ─── TOC sidebar ──────────────────────────────────────────────────────────────
static void draw_toc(int row, int col, int rows, int w,
                     const std::vector<TocEntry> &toc, int scroll) {
    for (int r = row; r < row + rows; r++) {
        gotoxy(r, col + w - 1);
        printf("%s%s│%s", C::bar, C::dim, C::reset);
    }
    gotoxy(row, col);
    printf("%s%s contents%s", C::bar, C::cyan, C::reset);
    printf("%s", C::bar);
    for (int i = 8; i < w - 1; i++) printf(" ");
    printf("%s", C::reset);

    gotoxy(row + 1, col);
    printf("%s%s", C::bar, C::dim);
    for (int i = 0; i < w - 1; i++) printf("─");
    printf("%s", C::reset);

    for (int i = 0; i < rows - 2; i++) {
        int idx = scroll + i;
        gotoxy(row + 2 + i, col);
        printf("%s", C::bar);
        if (idx < (int)toc.size()) {
            int indent = std::min(toc[idx].depth * 2, w / 2);
            printf("%s%*s%s%s", C::dim, indent, "", C::text, fit(toc[idx].title, w - 1 - indent).c_str());
        } else {
            for (int j = 0; j < w - 1; j++) printf(" ");
        }
        printf("%s", C::reset);
    }
    fflush(stdout);
}

// ─── Render dispatch ──────────────────────────────────────────────────────────
static void render_view(poppler::document *doc,
                        const poppler::page_renderer &rend,
                        const std::string &filepath,
                        int cur, int total, double zoom,
                        bool spread, bool toc_on,
                        const std::vector<TocEntry> &toc, int toc_scroll,
                        bool jump_mode, const std::string &jump_input,
                        bool pdf_dirty, RenderCache &cache) {
    auto ts = get_term_size();
    int img_rows = std::max(1, ts.rows - 2);
    int img_col  = 1;
    int img_cols = ts.cols;

    const int TOC_W    = 28;
    bool      has_toc  = toc_on && !toc.empty();
    if (has_toc) { img_col = TOC_W + 1; img_cols = ts.cols - TOC_W; }

    if (pdf_dirty || !cache.matches(cur, zoom, spread,
                                     img_col, img_cols, img_rows,
                                     ts.px_cols, ts.px_rows)) {
        double cw = (ts.px_cols > 0 && ts.cols > 0) ? (double)ts.px_cols / ts.cols : 9.0;
        double ch = (ts.px_rows > 0 && ts.rows > 0) ? (double)ts.px_rows / ts.rows : 18.0;
        auto png = make_png(doc, rend, cur, zoom, img_cols * cw, img_rows * ch, spread);
        if (!png.empty())
            cache.store(cur, zoom, spread, img_col, img_cols, img_rows,
                        ts.px_cols, ts.px_rows, std::move(png));
    }

    printf("\033[2J");
    draw_header(ts.cols, filepath);
    if (has_toc) draw_toc(2, 1, img_rows, TOC_W, toc, toc_scroll);
    if (!cache.b64_str.empty()) {
        printf("\033[2;%dH\033]1337;File=inline=1;width=%d;height=%d;"
               "preserveAspectRatio=1:%s\007",
               img_col, img_cols, img_rows, cache.b64_str.c_str());
    }
    draw_status(ts.rows, ts.cols, cur + 1, total, zoom, spread, jump_mode, jump_input);
    fflush(stdout);
}

// ─── Viewer loop ─────────────────────────────────────────────────────────────
void run_viewer(const std::string &filepath) {
    auto doc = std::unique_ptr<poppler::document>(
        poppler::document::load_from_file(filepath));
    if (!doc) {
        printf("\033[2J\033[H%sCannot open: %s%s\n", C::red, filepath.c_str(), C::reset);
        fflush(stdout);
        read_byte(2000);
        return;
    }
    int total = doc->pages();
    if (total == 0) return;

    poppler::page_renderer rend;
    rend.set_render_hints(poppler::page_renderer::antialiasing      |
                          poppler::page_renderer::text_antialiasing |
                          poppler::page_renderer::text_hinting);

    auto toc = load_toc(doc.get());
    save_to_history(filepath);

    int         cur = 0, toc_scroll = 0;
    double      zoom = 1.0;
    bool        spread = false, toc_on = false, show_help = false;
    bool        jump_mode = false;
    std::string jump_input;
    bool        pdf_dirty = true, ui_dirty = true;
    RenderCache cache;

    for (;;) {
        if (g_resized) { g_resized = 0; pdf_dirty = true; ui_dirty = true; }

        if (ui_dirty) {
            ui_dirty = false;
            render_view(doc.get(), rend, filepath,
                        cur, total, zoom, spread, toc_on, toc, toc_scroll,
                        jump_mode, jump_input, pdf_dirty, cache);
            pdf_dirty = false;
            if (show_help) draw_help(get_term_size());
            fflush(stdout);
        }

        int raw = read_byte(100);
        if (raw < 0) continue;
        int key = decode_key(raw);

        // ── Jump mode ─────────────────────────────────────────────────────
        if (jump_mode) {
            if (key == KEY_ENTER || key == '\r') {
                if (!jump_input.empty()) {
                    int next = std::max(0, std::min((int)std::stoi(jump_input) - 1, total - 1));
                    if (next != cur) { cur = next; pdf_dirty = true; }
                }
                jump_mode = false; jump_input.clear(); ui_dirty = true;
            } else if (key == KEY_ESC || key == KEY_BACKSPACE) {
                if (!jump_input.empty()) jump_input.pop_back();
                else                     jump_mode = false;
                ui_dirty = true;
            } else if (raw >= '0' && raw <= '9') {
                jump_input += char(raw); ui_dirty = true;
            }
            continue;
        }

        // ── Normal mode ───────────────────────────────────────────────────
        int step = spread ? 2 : 1;
        switch (key) {
            case 'q': case 'Q': case 3: return;

            case '?':
                show_help = !show_help; ui_dirty = true; break;
            case KEY_ESC:
                if (show_help) { show_help = false; ui_dirty = true; } break;
            case ':':
                jump_mode = true; jump_input.clear(); ui_dirty = true; break;

            case 's': case 'S':
                spread = !spread;
                if (spread && cur % 2 != 0) cur = std::max(0, cur - 1);
                pdf_dirty = true; ui_dirty = true; break;
            case 't': case 'T':
                if (!toc.empty()) { toc_on = !toc_on; pdf_dirty = true; ui_dirty = true; }
                break;

            case KEY_RIGHT: case 'l': case 'n': case ' ': case KEY_PGDN:
                if (cur + step < total) { cur += step; pdf_dirty = true; ui_dirty = true; } break;
            case KEY_LEFT: case 'h': case 'p': case KEY_PGUP:
                if (cur - step >= 0)    { cur -= step; pdf_dirty = true; ui_dirty = true; } break;
            case 'g': case KEY_HOME:
                if (cur) { cur = 0; pdf_dirty = true; ui_dirty = true; } break;
            case 'G': case KEY_END:
                cur = spread ? (total - 1) & ~1 : total - 1;
                pdf_dirty = true; ui_dirty = true; break;

            case '+': case '=': zoom += 0.25; pdf_dirty = true; ui_dirty = true; break;
            case '-': if (zoom > 0.25) { zoom -= 0.25; pdf_dirty = true; ui_dirty = true; } break;
            case '0': zoom = 1.0; pdf_dirty = true; ui_dirty = true; break;

            case KEY_UP:
                if (toc_on && !toc.empty() && toc_scroll > 0)
                    { toc_scroll--; ui_dirty = true; } break;
            case KEY_DOWN:
                if (toc_on && !toc.empty() && toc_scroll < (int)toc.size() - 1)
                    { toc_scroll++; ui_dirty = true; } break;
        }
    }
}
