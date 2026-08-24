#pragma once
#include <string>
#include "term.h"

// Move terminal cursor to (row, col) — 1-indexed.
void gotoxy(int row, int col);

// Pad or truncate s to exactly w columns (appends "…" when truncating).
std::string fit(const std::string &s, int w);

// Replace $HOME prefix with ~ for compact display.
std::string short_path(const std::string &path);

// Draw a rounded box filled with the panel background color.
// Optional title is shown in the top border.
// Returns the first inner content row.
int draw_box(int row, int col, int w, int h,
             const char *title     = nullptr,
             const char *title_col = nullptr);
