# pdfview

Terminal PDF viewer with inline image rendering for iTerm2. Renders PDF pages as images directly in your terminal with a Tokyo Night-themed TUI.

**Requires iTerm2** (or any terminal supporting the iTerm2 inline image protocol).

## Features

- Renders PDF pages as images inside the terminal
- Two-page spread mode (like macOS Preview)
- Table of contents sidebar
- Zoom in/out
- Jump to page
- Recent files home screen
- Tokyo Night color theme

## Install

### Homebrew (macOS / Linux)

```bash
brew tap l3chugu1t4/tap
brew install pdfview
```

### Build from source

**Dependencies:** `poppler`, `libpng`, `pkg-config`

```bash
# macOS
brew install poppler libpng pkg-config

# Ubuntu/Debian
sudo apt install libpoppler-cpp-dev libpng-dev pkg-config
```

```bash
git clone https://github.com/l3chugu1t4/pdfview
cd pdfview
make
sudo make install
```

## Usage

```bash
pdfview document.pdf   # open a file directly
pdfview               # open home screen
```

## Keybindings

| Key | Action |
|-----|--------|
| `→` `l` `Space` | Next page |
| `←` `h` | Previous page |
| `g` / `G` | First / last page |
| `:N` `Enter` | Jump to page N |
| `+` / `-` | Zoom in / out |
| `0` | Reset zoom |
| `s` | Toggle spread (two-page view) |
| `t` | Toggle table of contents |
| `?` | Toggle help |
| `q` | Quit |

## Requirements

- macOS or Linux
- iTerm2 (or terminal with iTerm2 inline image protocol support)
- C++17 compiler
