// Direct2D + DirectWrite drawing backend for ui_gdi.
//
// C ABI; implemented in src/ui/dxd.cpp. Coordinates are int32_t pixels (a
// float coordinate system is a later change). This header is included by
// ui/ui.h AFTER the ui types, so the amalgamation self-contains it -- do
// not include ui.h here (would be circular).
//
// Design: GDI stays the source of truth for fonts (ui_font_t == HFONT) and
// for image pixel buffers (DIB sections); dxd only *draws*. dxd_text derives
// a DirectWrite format from the HFONT's LOGFONT, and dxd_image uploads the
// CPU pixel buffer into a transient Direct2D bitmap. So ui_app font creation,
// ui_image bitmaps, and clipboard interop are unchanged.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dxd_context_s * dxd_context_t;

bool dxd_init(void);
void dxd_fini(void);

// `hdc` is the target device context (window canvas or an offscreen memory
// DC); `rc` is the bound region in pixels.
dxd_context_t dxd_begin(void * hdc, const ui_rect_t * rc);
void          dxd_end(dxd_context_t ctx);

void dxd_set_clip(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h);
void dxd_pixel(dxd_context_t ctx, int32_t x, int32_t y, ui_color_t color);
void dxd_line(dxd_context_t ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
              ui_color_t color);
void dxd_frame(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h,
               ui_color_t color);
void dxd_rect(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h,
              ui_color_t border, ui_color_t fill);
void dxd_fill(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h,
              ui_color_t color);
void dxd_poly(dxd_context_t ctx, ui_point_t * points, int32_t count,
              ui_color_t color);
void dxd_circle(dxd_context_t ctx, int32_t x, int32_t y, int32_t radius,
                ui_color_t border, ui_color_t fill);
void dxd_rounded(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h,
                 int32_t radius, ui_color_t border, ui_color_t fill);
void dxd_gradient(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h,
                  ui_color_t c0, ui_color_t c1, bool vertical);

// Image blit from a CPU pixel buffer. bpp: 1 = greyscale, 3 = BGR (stride
// padded to 4), 4 = BGRA. Scales source (sx,sy,sw,sh) into destination
// (dx,dy,dw,dh); `opacity` in [0,1] applies a global alpha.
void dxd_image(dxd_context_t ctx, int32_t dx, int32_t dy, int32_t dw, int32_t dh,
               int32_t sx, int32_t sy, int32_t sw, int32_t sh,
               int32_t w, int32_t h, int32_t stride, int32_t bpp,
               const uint8_t * pixels, fp64_t opacity, bool premultiplied);

// Text. `font` is the GDI ui_font_t (HFONT); a DirectWrite text format is
// derived from its LOGFONT. `measure_only` skips drawing. `w` > 0 with
// `multiline` wraps; otherwise single line. Returns the measured size.
ui_wh_t dxd_text(dxd_context_t ctx, ui_font_t font, int32_t x, int32_t y,
                 int32_t w, ui_color_t color, const char * utf8, int32_t bytes,
                 bool measure_only, bool multiline);
ui_wh_t dxd_glyphs_placement(ui_font_t font, const char * utf8, int32_t bytes,
                             int32_t x_out[], int32_t glyphs);

#ifdef __cplusplus
}
#endif
