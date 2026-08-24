#include "term.h"

#include <cstdio>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

volatile sig_atomic_t g_resized = 0;
static struct termios  g_orig_termios;

void sig_handler(int s) {
    restore_term();
    if (s == SIGINT) raise(SIGINT);
    _exit(0);
}

TermSize get_term_size() {
    struct winsize ws = {};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return {ws.ws_row, ws.ws_col, ws.ws_ypixel, ws.ws_xpixel};
}

void enable_raw() {
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    struct termios r = g_orig_termios;
    r.c_iflag &= ~(ICRNL | IXON);
    r.c_lflag &= ~(ECHO | ICANON);
    r.c_cc[VMIN] = 1; r.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &r);
}

void restore_term() {
    printf("\033[?1049l\033[?25h");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
}

int read_byte(int ms) {
    fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = {ms / 1000, (ms % 1000) * 1000};
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, ms >= 0 ? &tv : nullptr) <= 0)
        return -1;
    unsigned char c;
    return read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
}

int decode_key(int c) {
    if (c != 27) return c;
    int s0 = read_byte(50), s1 = read_byte(50);
    if (s0 == '[') {
        switch (s1) {
            case 'A': return KEY_UP;    case 'B': return KEY_DOWN;
            case 'C': return KEY_RIGHT; case 'D': return KEY_LEFT;
            case 'H': return KEY_HOME;  case 'F': return KEY_END;
        }
        if (s1 >= '1' && s1 <= '6') {
            int s2 = read_byte(50);
            if (s2 == '~') switch (s1) {
                case '3': return KEY_DEL;  case '5': return KEY_PGUP;
                case '6': return KEY_PGDN; case '1': return KEY_HOME;
                case '4': return KEY_END;
            }
        }
    }
    return KEY_ESC;
}
