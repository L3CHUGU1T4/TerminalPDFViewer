#include "term.h"
#include "history.h"
#include "home.h"
#include "viewer.h"

#include <csignal>
#include <cstdio>

int main(int argc, char *argv[]) {
    enable_raw();
    signal(SIGWINCH, [](int) { g_resized = 1; });
    signal(SIGINT,   sig_handler);
    signal(SIGTERM,  sig_handler);
    printf("\033[?1049h\033[?25l");
    fflush(stdout);

    auto history = load_history();

    if (argc >= 2) {
        run_viewer(argv[1]);
    } else {
        for (;;) {
            std::string path = run_home(history);
            if (path.empty()) break;
            run_viewer(path);
            history = load_history();
        }
    }

    restore_term();
    return 0;
}
