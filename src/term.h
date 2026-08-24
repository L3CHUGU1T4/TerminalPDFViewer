#pragma once
#include <csignal>
#include <string>

// ─── Key codes ────────────────────────────────────────────────────────────────
enum Key {
    KEY_UNKNOWN   = -2,
    KEY_TIMEOUT   = -1,
    KEY_ENTER     = 13,
    KEY_ESC       = 27,
    KEY_BACKSPACE = 127,
    KEY_LEFT  = 1000, KEY_RIGHT, KEY_UP, KEY_DOWN,
    KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDN, KEY_DEL,
};

// ─── Tokyo Night color theme ──────────────────────────────────────────────────
namespace C {
    constexpr const char *bg     = "\033[48;2;26;27;38m";
    constexpr const char *panel  = "\033[48;2;31;35;53m";
    constexpr const char *bar    = "\033[48;2;36;40;59m";
    constexpr const char *sel    = "\033[48;2;61;66;86m";
    constexpr const char *text   = "\033[38;2;192;202;245m";
    constexpr const char *dim    = "\033[38;2;86;95;137m";
    constexpr const char *bright = "\033[38;2;240;246;252m";
    constexpr const char *blue   = "\033[38;2;122;162;247m";
    constexpr const char *purple = "\033[38;2;187;154;247m";
    constexpr const char *green  = "\033[38;2;158;206;106m";
    constexpr const char *cyan   = "\033[38;2;125;207;255m";
    constexpr const char *red    = "\033[38;2;247;118;142m";
    constexpr const char *orange = "\033[38;2;255;158;100m";
    constexpr const char *reset  = "\033[0m";
    constexpr const char *bold   = "\033[1m";
}

// ─── Terminal ─────────────────────────────────────────────────────────────────
struct TermSize { int rows, cols, px_rows, px_cols; };

extern volatile sig_atomic_t g_resized;

void     sig_handler(int sig);
TermSize get_term_size();
void     enable_raw();
void     restore_term();
int      read_byte(int timeout_ms);
int      decode_key(int c);
