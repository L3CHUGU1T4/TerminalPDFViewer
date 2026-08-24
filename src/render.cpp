#include "render.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>

#include <png.h>
#include <poppler-page.h>
#include <poppler-toc.h>

// ─── TOC ──────────────────────────────────────────────────────────────────────
std::vector<TocEntry> load_toc(poppler::document *doc) {
    std::vector<TocEntry> entries;
    auto toc = std::unique_ptr<poppler::toc>(doc->create_toc());
    if (!toc) return entries;

    std::function<void(poppler::toc_item *, int)> walk =
        [&](poppler::toc_item *item, int depth) {
            for (auto *child : item->children()) {
                TocEntry e;
                auto ba = child->title().to_utf8();
                e.title = std::string(ba.begin(), ba.end());
                e.depth = depth;
                entries.push_back(e);
                walk(child, depth + 1);
            }
        };
    walk(toc->root(), 0);
    return entries;
}

// ─── Base64 ───────────────────────────────────────────────────────────────────
static const char kB64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string b64(const uint8_t *d, size_t n) {
    std::string o;
    o.reserve(((n + 2) / 3) * 4);
    for (size_t i = 0; i < n; i += 3) {
        uint32_t b = (uint32_t)d[i] << 16;
        if (i + 1 < n) b |= (uint32_t)d[i + 1] << 8;
        if (i + 2 < n) b |= d[i + 2];
        o += kB64[(b >> 18) & 63];
        o += kB64[(b >> 12) & 63];
        o += (i + 1 < n) ? kB64[(b >> 6) & 63] : '=';
        o += (i + 2 < n) ? kB64[b & 63] : '=';
    }
    return o;
}

// ─── PNG encoder ─────────────────────────────────────────────────────────────
namespace {
    struct PNGBuf { std::vector<uint8_t> data; };
    void png_write_cb(png_structp p, png_bytep d, png_size_t n) {
        auto *buf = static_cast<PNGBuf *>(png_get_io_ptr(p));
        buf->data.insert(buf->data.end(), d, d + n);
    }
}

std::vector<uint8_t> rgb_to_png(const uint8_t *rgb, int w, int h) {
    auto *png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) return {};
    auto *info = png_create_info_struct(png);
    if (!info) { png_destroy_write_struct(&png, nullptr); return {}; }

    PNGBuf buf;
    if (setjmp(png_jmpbuf(png))) { png_destroy_write_struct(&png, &info); return {}; }

    png_set_write_fn(png, &buf, png_write_cb, nullptr);
    png_set_IHDR(png, info, w, h, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_set_compression_level(png, 1);
    png_write_info(png, info);
    for (int y = 0; y < h; y++)
        png_write_row(png, const_cast<uint8_t *>(rgb + y * w * 3));
    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
    return buf.data;
}

// ─── Image blit ───────────────────────────────────────────────────────────────
void blit(poppler::image img, std::vector<uint8_t> &out,
          int bw, int bh, int dx, int dy) {
    auto fmt        = img.format();
    const auto *src = reinterpret_cast<const uint8_t *>(img.data());
    int stride      = img.bytes_per_row();
    int iw = img.width(), ih = img.height();

    for (int y = 0; y < ih; y++) {
        int oy = dy + y;
        if (oy < 0 || oy >= bh) continue;
        for (int x = 0; x < iw; x++) {
            int ox = dx + x;
            if (ox < 0 || ox >= bw) continue;
            uint8_t r, g, bl;
            if (fmt == poppler::image::format_argb32) {
                uint32_t px = reinterpret_cast<const uint32_t *>(src + y * stride)[x];
                r = (px >> 16) & 0xff;
                g = (px >>  8) & 0xff;
                bl =  px       & 0xff;
            } else {
                const uint8_t *p = src + y * stride + x * 3;
                r = p[0]; g = p[1]; bl = p[2];
            }
            size_t i = ((size_t)oy * bw + ox) * 3;
            out[i] = r; out[i + 1] = g; out[i + 2] = bl;
        }
    }
}

// ─── PDF rendering ────────────────────────────────────────────────────────────
static double calc_dpi(double pw, double ph, double aw, double ah, double zoom) {
    double dw = (pw > 0) ? (aw / pw) * 72.0 : 150.0;
    double dh = (ph > 0) ? (ah / ph) * 72.0 : 150.0;
    return std::max(36.0, std::min(std::min(dw, dh) * zoom, 600.0));
}

static poppler::image render_page_img(poppler::document *doc,
                                       const poppler::page_renderer &rend,
                                       int idx, double dpi) {
    auto p = std::unique_ptr<poppler::page>(doc->create_page(idx));
    return p ? rend.render_page(p.get(), dpi, dpi) : poppler::image{};
}

static std::pair<double, double> page_pts(poppler::document *doc, int idx) {
    auto p = std::unique_ptr<poppler::page>(doc->create_page(idx));
    if (!p) return {612, 792};
    auto r = p->page_rect(poppler::media_box);
    return {r.width(), r.height()};
}

std::vector<uint8_t> make_png(poppler::document *doc,
                               const poppler::page_renderer &rend,
                               int idx, double zoom,
                               double avail_w_px, double avail_h_px,
                               bool spread) {
    const int GAP = 8;
    auto [lw, lh] = page_pts(doc, idx);
    double page_w  = spread ? (avail_w_px - GAP) / 2.0 : avail_w_px;
    double dpi_l   = calc_dpi(lw, lh, page_w, avail_h_px, zoom);
    auto img_l     = render_page_img(doc, rend, idx, dpi_l);
    if (!img_l.is_valid()) return {};

    if (!spread || idx + 1 >= doc->pages()) {
        int w = img_l.width(), h = img_l.height();
        std::vector<uint8_t> rgb(w * h * 3, 0xff);
        blit(img_l, rgb, w, h, 0, 0);
        return rgb_to_png(rgb.data(), w, h);
    }

    auto [rw, rh] = page_pts(doc, idx + 1);
    double dpi_r   = calc_dpi(rw, rh, page_w, avail_h_px, zoom);
    auto img_r     = render_page_img(doc, rend, idx + 1, dpi_r);

    int w1 = img_l.width(), h1 = img_l.height();
    int w2 = img_r.is_valid() ? img_r.width()  : 0;
    int h2 = img_r.is_valid() ? img_r.height() : 0;
    int W  = w1 + (w2 > 0 ? GAP + w2 : 0);
    int H  = std::max(h1, h2);
    std::vector<uint8_t> rgb(W * H * 3, 0xff);
    blit(img_l, rgb, W, H, 0, 0);
    if (w2 > 0) blit(img_r, rgb, W, H, w1 + GAP, 0);
    return rgb_to_png(rgb.data(), W, H);
}

// ─── RenderCache ─────────────────────────────────────────────────────────────
bool RenderCache::matches(int p, double z, bool s,
                           int ic, int ico, int ir, int pc, int pr) const {
    return page == p && spread == s && std::abs(zoom - z) < 0.001 &&
           img_col == ic && img_cols == ico && img_rows == ir &&
           px_cols == pc && px_rows == pr;
}

void RenderCache::store(int p, double z, bool s,
                         int ic, int ico, int ir, int pc, int pr,
                         std::vector<uint8_t> data) {
    page = p; zoom = z; spread = s;
    img_col = ic; img_cols = ico; img_rows = ir;
    px_cols = pc; px_rows = pr;
    png     = std::move(data);
    b64_str = b64(png.data(), png.size());
}
