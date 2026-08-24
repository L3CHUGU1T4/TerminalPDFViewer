#include "history.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>

static std::string history_file() {
    const char *h = getenv("HOME");
    if (!h) return "";
    return std::string(h) + "/.local/share/pdfview_history";
}

std::vector<std::string> load_history() {
    std::vector<std::string> v;
    std::ifstream f(history_file());
    std::string line;
    while (std::getline(f, line) && v.size() < 8)
        if (!line.empty()) v.push_back(line);
    return v;
}

void save_to_history(const std::string &path) {
    auto hist = load_history();
    hist.erase(std::remove(hist.begin(), hist.end(), path), hist.end());
    hist.insert(hist.begin(), path);
    if (hist.size() > 8) hist.resize(8);

    std::string fp = history_file();
    size_t slash = fp.rfind('/');
    if (slash != std::string::npos)
        mkdir(fp.substr(0, slash).c_str(), 0755);

    std::ofstream f(fp);
    for (auto &e : hist) f << e << '\n';
}
