#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <poppler-document.h>
#include <poppler-image.h>
#include <poppler-page-renderer.h>

// ─── Table of contents entry ─────────────────────────────────────────────────
struct TocEntry { std::string title; int depth; };

std::vector<TocEntry> load_toc(poppler::document *doc);

// ─── Encoding ─────────────────────────────────────────────────────────────────
std::string          b64(const uint8_t *data, size_t len);
std::vector<uint8_t> rgb_to_png(const uint8_t *rgb, int w, int h);
void                 blit(poppler::image img, std::vector<uint8_t> &out,
                          int bw, int bh, int dx, int dy);

// ─── PDF → PNG ────────────────────────────────────────────────────────────────
std::vector<uint8_t> make_png(poppler::document *doc,
                               const poppler::page_renderer &rend,
                               int idx, double zoom,
                               double avail_w_px, double avail_h_px,
                               bool spread);

// ─── Render cache ─────────────────────────────────────────────────────────────
// Avoids re-rendering the PDF when only UI elements changed.
struct RenderCache {
    int    page    = -1;
    double zoom    = -1.0;
    bool   spread  = false;
    int    img_col = 0, img_cols = 0, img_rows = 0;
    int    px_cols = 0, px_rows  = 0;
    std::vector<uint8_t> png;
    std::string          b64_str;   // kept to skip re-encoding on UI redraws

    bool matches(int p, double z, bool s,
                 int ic, int ico, int ir, int pc, int pr) const;
    void store(int p, double z, bool s,
               int ic, int ico, int ir, int pc, int pr,
               std::vector<uint8_t> data);
    void invalidate() { page = -1; }
};
