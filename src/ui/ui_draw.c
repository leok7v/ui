#include "posix/posix.h"
#include "ui/ui.h"
#include "ui/ui_win32.h"

#pragma push_macro("ui_draw_with_hdc")
#pragma push_macro("ui_draw_hdc_with_font")

static ui_brush_t  ui_draw_brush_hollow;
static ui_brush_t  ui_draw_brush_color;
static ui_pen_t    ui_draw_pen_hollow;
static ui_region_t ui_draw_clip;

struct ui_draw_context {
    HDC hdc; // window canvas() or memory DC
    int32_t background_mode;
    int32_t stretch_mode;
    ui_pen_t pen;
    ui_font_t font;
    ui_color_t text_color;
    POINT brush_origin;
    ui_brush_t brush;
    HBITMAP texture;
    dxd_context_t dxd; // Direct2D draw context for this begin()/end() frame
};

static struct ui_draw_context ui_draw_context;

#define ui_draw_hdc() (ui_draw_context.hdc)

static void ui_draw_init(void) {
    dxd_init();
}

static void ui_draw_fini(void) {
    dxd_fini();
}

static ui_pen_t ui_draw_set_pen(ui_pen_t p) {
    posix_not_null(p);
    return (ui_pen_t)SelectPen(ui_draw_hdc(), (HPEN)p);
}

static ui_brush_t ui_draw_set_brush(ui_brush_t b) {
    posix_not_null(b);
    return (ui_brush_t)SelectBrush(ui_draw_hdc(), b);
}

static uint32_t ui_draw_color_rgb(ui_color_t c) {
    posix_assert(ui_color_is_8bit(c));
    return (COLORREF)(c & 0xFFFFFFFF);
}

static COLORREF ui_draw_color_ref(ui_color_t c) {
    return ui_draw.color_rgb(c);
}

static ui_color_t ui_draw_set_text_color(ui_color_t c) {
    return SetTextColor(ui_draw_hdc(), ui_draw_color_ref(c));
}

static ui_font_t ui_draw_set_font(ui_font_t f) {
    posix_not_null(f);
    return (ui_font_t)SelectFont(ui_draw_hdc(), (HFONT)f);
}

static void ui_draw_begin(struct ui_bitmap* image) {
    posix_swear(ui_draw_context.hdc == null, "no nested begin()/end()");
    if (image != null) {
        posix_swear(image->texture != null);
        ui_draw_context.hdc = CreateCompatibleDC((HDC)ui_app.canvas);
        ui_draw_context.texture = SelectBitmap(ui_draw_hdc(),
                                             (HBITMAP)image->texture);
    } else {
        ui_draw_context.hdc = (HDC)ui_app.canvas;
        posix_swear(ui_draw_context.texture == null);
    }
    ui_draw_context.text_color = ui_colors.get_color(ui_color_id_window_text);
    struct ui_rect rc = image != null ?
        (struct ui_rect){ 0, 0, image->w, image->h } :
        (struct ui_rect){ 0, 0, ui_app.crc.w, ui_app.crc.h };
    ui_draw_context.dxd = dxd_begin(ui_draw_context.hdc, &rc);
}

static void ui_draw_end(void) {
    if (ui_draw_context.dxd != null) {
        dxd_end(ui_draw_context.dxd);
        ui_draw_context.dxd = null;
    }
    if (ui_draw_context.hdc != (HDC)ui_app.canvas) {
        posix_swear(ui_draw_context.texture != null); // 1x1 bitmap
        SelectBitmap(ui_draw_context.hdc, (HBITMAP)ui_draw_context.texture);
        posix_fatal_win32err(DeleteDC(ui_draw_context.hdc));
    }
    memset(&ui_draw_context, 0x00, sizeof(ui_draw_context));
}

static ui_pen_t ui_draw_set_colored_pen(ui_color_t c) {
    ui_pen_t p = (ui_pen_t)SelectPen(ui_draw_hdc(), GetStockPen(DC_PEN));
    SetDCPenColor(ui_draw_hdc(), ui_draw_color_ref(c));
    return p;
}

static ui_pen_t ui_draw_create_pen(ui_color_t c, int32_t width) {
    posix_assert(width >= 1);
    ui_pen_t pen = (ui_pen_t)CreatePen(PS_SOLID, width, ui_draw_color_ref(c));
    posix_not_null(pen);
    return pen;
}

static void ui_draw_delete_pen(ui_pen_t p) {
    posix_fatal_win32err(DeletePen(p));
}

static ui_brush_t ui_draw_create_brush(ui_color_t c) {
    return (ui_brush_t)CreateSolidBrush(ui_draw_color_ref(c));
}

static void ui_draw_delete_brush(ui_brush_t b) {
    DeleteBrush((HBRUSH)b);
}

static ui_color_t ui_draw_set_brush_color(ui_color_t c) {
    return SetDCBrushColor(ui_draw_hdc(), ui_draw_color_ref(c));
}

static void ui_draw_set_clip(int32_t x, int32_t y, int32_t w, int32_t h) {
    dxd_set_clip(ui_draw_context.dxd, x, y, w, h);
}

static void ui_draw_pixel(int32_t x, int32_t y, ui_color_t c) {
    dxd_pixel(ui_draw_context.dxd, x, y, c);
}

static void ui_draw_rectangle(int32_t x, int32_t y, int32_t w, int32_t h) {
    posix_fatal_win32err(Rectangle(ui_draw_hdc(), x, y, x + w, y + h));
}

static void ui_draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
        ui_color_t c) {
    dxd_line(ui_draw_context.dxd, x0, y0, x1, y1, c);
}

static void ui_draw_frame(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
    dxd_frame(ui_draw_context.dxd, x, y, w, h, c);
}

static void ui_draw_rect(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t border, ui_color_t fill) {
    dxd_rect(ui_draw_context.dxd, x, y, w, h, border, fill);
}

static void ui_draw_fill(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
    dxd_fill(ui_draw_context.dxd, x, y, w, h, c);
}

static void ui_draw_poly(struct ui_point* points, int32_t count, ui_color_t c) {
    dxd_poly(ui_draw_context.dxd, points, count, c);
}

static void ui_draw_circle(int32_t x, int32_t y, int32_t radius,
        ui_color_t border, ui_color_t fill) {
    dxd_circle(ui_draw_context.dxd, x, y, radius, border, fill);
}

static void ui_draw_fill_rounded(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t radius, ui_color_t fill) {
    int32_t r = x + w - 1; // right
    int32_t b = y + h - 1; // bottom
    ui_draw_circle(x + radius, y + radius, radius, fill, fill);
    ui_draw_circle(r - radius, y + radius, radius, fill, fill);
    ui_draw_circle(x + radius, b - radius, radius, fill, fill);
    ui_draw_circle(r - radius, b - radius, radius, fill, fill);
    // rectangles
    ui_draw.fill(x + radius, y, w - radius * 2, h, fill);
    r = x + w - radius;
    ui_draw.fill(x, y + radius, radius, h - radius * 2, fill);
    ui_draw.fill(r, y + radius, radius, h - radius * 2, fill);
}

static void ui_draw_rounded_border(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t radius, ui_color_t border) {
    {
        int32_t r = x + w - 1; // right
        int32_t b = y + h - 1; // bottom
        ui_draw.set_clip(x, y, radius + 1, radius + 1);
        ui_draw_circle(x + radius, y + radius, radius, border, ui_colors.transparent);
        ui_draw.set_clip(r - radius, y, radius + 1, radius + 1);
        ui_draw_circle(r - radius, y + radius, radius, border, ui_colors.transparent);
        ui_draw.set_clip(x, b - radius, radius + 1, radius + 1);
        ui_draw_circle(x + radius, b - radius, radius, border, ui_colors.transparent);
        ui_draw.set_clip(r - radius, b - radius, radius + 1, radius + 1);
        ui_draw_circle(r - radius, b - radius, radius, border, ui_colors.transparent);
        ui_draw.set_clip(0, 0, 0, 0);
    }
    {
        int32_t r = x + w - 1; // right
        int32_t b = y + h - 1; // bottom
        ui_draw.line(x + radius, y, r - radius + 1, y, border);
        ui_draw.line(x + radius, b, r - radius + 1, b, border);
        ui_draw.line(x - 1, y + radius, x - 1, b - radius + 1, border);
        ui_draw.line(r + 1, y + radius, r + 1, b - radius + 1, border);
    }
}

static void ui_draw_rounded(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t radius, ui_color_t border, ui_color_t fill) {
    dxd_rounded(ui_draw_context.dxd, x, y, w, h, radius, border, fill);
}

static void ui_draw_gradient(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t rgba_from, ui_color_t rgba_to, bool vertical) {
    dxd_gradient(ui_draw_context.dxd, x, y, w, h, rgba_from, rgba_to, vertical);
}

static BITMAPINFO* ui_draw_greyscale_bitmap_info(void) {
    typedef struct bitmap_rgb_s {
        BITMAPINFO bi;
        RGBQUAD rgb[256];
    } bitmap_rgb_t;
    static bitmap_rgb_t storage; // for gs palette
    static BITMAPINFO* bi = &storage.bi;
    BITMAPINFOHEADER* bih = &bi->bmiHeader;
    if (bih->biSize == 0) { // once
        bih->biSize = sizeof(BITMAPINFOHEADER);
        for (int32_t i = 0; i < 256; i++) {
            RGBQUAD* q = &bi->bmiColors[i];
            q->rgbReserved = 0;
            q->rgbBlue = q->rgbGreen = q->rgbRed = (uint8_t)i;
        }
        bih->biPlanes = 1;
        bih->biBitCount = 8;
        bih->biCompression = BI_RGB;
        bih->biClrUsed = 256;
        bih->biClrImportant = 256;
    }
    return bi;
}

static void ui_draw_pixels(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        int32_t width, int32_t height, int32_t stride,
        int32_t bpp, const uint8_t* pixels) {
    if (bpp == 1) {
        ui_draw.greyscale(dx, dy, dw, dh, ix, iy, iw, ih, width, height, stride, pixels);
    } else if (bpp == 3) {
        ui_draw.bgr(dx, dy, dw, dh, ix, iy, iw, ih, width, height, stride, pixels);
    } else if (bpp == 4) {
        ui_draw.bgrx(dx, dy, dw, dh, ix, iy, iw, ih, width, height, stride, pixels);
    } else {
        posix_fatal("bpp: %d not {1, 3, 4}", bpp);
    }
}

static void ui_draw_greyscale(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        int32_t width, int32_t height, int32_t stride, const uint8_t* pixels) {
    dxd_image(ui_draw_context.dxd, dx, dy, dw, dh, ix, iy, iw, ih,
              width, height, stride, 1, pixels, 1.0, false);
}

static BITMAPINFOHEADER ui_draw_bgrx_init_bi(int32_t w, int32_t h, int32_t bpp) {
    posix_assert(w > 0 && h >= 0); // h cannot be negative?
    BITMAPINFOHEADER bi = {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biPlanes = 1,
        .biBitCount = (uint16_t)(bpp * 8),
        .biCompression = BI_RGB,
        .biWidth = w,
        .biHeight = -h, // top down image
        .biSizeImage = (DWORD)(w * abs(h) * bpp),
        .biClrUsed = 0,
        .biClrImportant = 0
   };
   return bi;
}

// bgr(width) assumes strides are padded and rounded up to 4 bytes
// if this is not the case use ui_draw.bitmap_init() that will unpack
// and align scanlines prior to draw

static void ui_draw_bgr(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        int32_t width, int32_t height, int32_t stride,
        const uint8_t* pixels) {
    dxd_image(ui_draw_context.dxd, dx, dy, dw, dh, ix, iy, iw, ih,
              width, height, stride, 3, pixels, 1.0, false);
}

static void ui_draw_bgrx(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        int32_t width, int32_t height, int32_t stride,
        const uint8_t* pixels) {
    dxd_image(ui_draw_context.dxd, dx, dy, dw, dh, ix, iy, iw, ih,
              width, height, stride, 4, pixels, 1.0, false);
}

static BITMAPINFO* ui_draw_init_bitmap_info(int32_t w, int32_t h, int32_t bpp,
        BITMAPINFO* bi) {
    posix_assert(w > 0 && h >= 0); // h cannot be negative?
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = w;
    bi->bmiHeader.biHeight = -h;  // top down image
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = (uint16_t)(bpp * 8);
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biSizeImage = (DWORD)(w * abs(h) * bpp);
    return bi;
}

static void ui_draw_create_dib_section(struct ui_bitmap* image, int32_t w, int32_t h,
        int32_t bpp) {
    posix_fatal_if(image->texture != null, "bitmap_dispose() not called?");
    // not using GetWindowDC(ui_app.window) will allow to initialize images
    // before window is created
    HDC c = CreateCompatibleDC(null); // GetWindowDC(ui_app.window);
    BITMAPINFO local = { {sizeof(BITMAPINFOHEADER)} };
    BITMAPINFO* bi = bpp == 1 ? ui_draw_greyscale_bitmap_info() : &local;
    image->texture = (ui_texture_t)CreateDIBSection(c, 
            ui_draw_init_bitmap_info(w, h, bpp, bi),
            DIB_RGB_COLORS, &image->pixels, null, 0x0
    );
    posix_fatal_if(image->texture == null || image->pixels == null);
    posix_fatal_win32err(DeleteDC(c));
}

static void ui_draw_bitmap_init_rgbx(struct ui_bitmap* image, int32_t w, int32_t h,
        int32_t bpp, const uint8_t* pixels) {
    bool swapped = bpp < 0;
    bpp = abs(bpp);
    posix_fatal_if(bpp != 4, "bpp: %d", bpp);
    ui_draw_create_dib_section(image, w, h, bpp);
    const int32_t stride = (w * bpp + 3) & ~0x3;
    uint8_t* scanline = image->pixels;
    const uint8_t* rgbx = pixels;
    if (!swapped) {
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgra = scanline;
            for (int32_t x = 0; x < w; x++) {
                bgra[0] = rgbx[2];
                bgra[1] = rgbx[1];
                bgra[2] = rgbx[0];
                bgra[3] = 0xFF;
                bgra += 4;
                rgbx += 4;
            }
            pixels += w * 4;
            scanline += stride;
        }
    } else {
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgra = scanline;
            for (int32_t x = 0; x < w; x++) {
                bgra[0] = rgbx[0];
                bgra[1] = rgbx[1];
                bgra[2] = rgbx[2];
                bgra[3] = 0xFF;
                bgra += 4;
                rgbx += 4;
            }
            pixels += w * 4;
            scanline += stride;
        }
    }
    image->w = w;
    image->h = h;
    image->bpp = bpp;
    image->stride = stride;
}

static void ui_draw_bitmap_init(struct ui_bitmap* image, int32_t w, int32_t h, int32_t bpp,
        const uint8_t* pixels) {
    bool swapped = bpp < 0;
    bpp = abs(bpp);
    posix_fatal_if(bpp < 0 || bpp == 2 || bpp > 4, "bpp=%d not {1, 3, 4}", bpp);
    ui_draw_create_dib_section(image, w, h, bpp);
    // Win32 bitmaps stride is rounded up to 4 bytes
    const int32_t stride = (w * bpp + 3) & ~0x3;
    uint8_t* scanline = image->pixels;
    if (bpp == 1) {
        for (int32_t y = 0; y < h; y++) {
            memcpy(scanline, pixels, (size_t)w);
            pixels += w;
            scanline += stride;
        }
    } else if (bpp == 3 && !swapped) {
        const uint8_t* rgb = pixels;
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgr = scanline;
            for (int32_t x = 0; x < w; x++) {
                bgr[0] = rgb[2];
                bgr[1] = rgb[1];
                bgr[2] = rgb[0];
                bgr += 3;
                rgb += 3;
            }
            pixels += w * bpp;
            scanline += stride;
        }
    } else if (bpp == 3 && swapped) {
        const uint8_t* rgb = pixels;
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgr = scanline;
            for (int32_t x = 0; x < w; x++) {
                bgr[0] = rgb[0];
                bgr[1] = rgb[1];
                bgr[2] = rgb[2];
                bgr += 3;
                rgb += 3;
            }
            pixels += w * bpp;
            scanline += stride;
        }
    } else if (bpp == 4 && !swapped) {
        // premultiply alpha, see:
        // https://stackoverflow.com/questions/24595717/alphablend-generating-incorrect-colors
        const uint8_t* rgba = pixels;
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgra = scanline;
            for (int32_t x = 0; x < w; x++) {
                int32_t alpha = rgba[3];
                bgra[0] = (uint8_t)(rgba[2] * alpha / 255);
                bgra[1] = (uint8_t)(rgba[1] * alpha / 255);
                bgra[2] = (uint8_t)(rgba[0] * alpha / 255);
                bgra[3] = rgba[3];
                bgra += 4;
                rgba += 4;
            }
            pixels += w * 4;
            scanline += stride;
        }
    } else if (bpp == 4 && swapped) {
        // premultiply alpha, see:
        // https://stackoverflow.com/questions/24595717/alphablend-generating-incorrect-colors
        const uint8_t* rgba = pixels;
        for (int32_t y = 0; y < h; y++) {
            uint8_t* bgra = scanline;
            for (int32_t x = 0; x < w; x++) {
                int32_t alpha = rgba[3];
                bgra[0] = (uint8_t)(rgba[0] * alpha / 255);
                bgra[1] = (uint8_t)(rgba[1] * alpha / 255);
                bgra[2] = (uint8_t)(rgba[2] * alpha / 255);
                bgra[3] = rgba[3];
                bgra += 4;
                rgba += 4;
            }
            pixels += w * 4;
            scanline += stride;
        }
    }
    image->w = w;
    image->h = h;
    image->bpp = bpp;
    image->stride = stride;
}

static void ui_draw_alpha(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        struct ui_bitmap* image, fp64_t alpha) {
    posix_assert(image->bpp > 0);
    posix_assert(0 <= alpha && alpha <= 1);
    dxd_image_cached(ui_draw_context.dxd, &image->dxd, dx, dy, dw, dh,
              ix, iy, iw, ih, image->w, image->h, image->stride, image->bpp,
              (const uint8_t*)image->pixels, alpha, true);
}

static void ui_draw_bitmap(int32_t dx, int32_t dy, int32_t dw, int32_t dh,
        int32_t ix, int32_t iy, int32_t iw, int32_t ih,
        struct ui_bitmap* image) {
    posix_assert(image->bpp == 1 || image->bpp == 3 || image->bpp == 4);
    dxd_image_cached(ui_draw_context.dxd, &image->dxd, dx, dy, dw, dh,
              ix, iy, iw, ih, image->w, image->h, image->stride, image->bpp,
              (const uint8_t*)image->pixels, 1.0, false);
}

static void ui_draw_icon(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_icon_t icon) {
    DrawIconEx(ui_draw_hdc(), x, y, (HICON)icon, w, h, 0, NULL, DI_NORMAL | DI_COMPAT);
}

static void ui_draw_cleartype(bool on) {
    enum { spif = SPIF_UPDATEINIFILE | SPIF_SENDCHANGE };
    posix_fatal_win32err(SystemParametersInfoA(SPI_SETFONTSMOOTHING,
                                                   true, 0, spif));
    uintptr_t s = on ? FE_FONTSMOOTHINGCLEARTYPE : FE_FONTSMOOTHINGSTANDARD;
    posix_fatal_win32err(SystemParametersInfoA(SPI_SETFONTSMOOTHINGTYPE,
        0, (void*)s, spif));
}

static void ui_draw_font_smoothing_contrast(int32_t c) {
    posix_fatal_if(!(c == -1 || 1000 <= c && c <= 2200), "contrast: %d", c);
    if (c == -1) { c = 1400; }
    posix_fatal_win32err(SystemParametersInfoA(SPI_SETFONTSMOOTHINGCONTRAST,
        0, (void*)(uintptr_t)c, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE));
}

posix_static_assertion(ui_font_quality_default == DEFAULT_QUALITY);
posix_static_assertion(ui_font_quality_draft == DRAFT_QUALITY);
posix_static_assertion(ui_font_quality_proof == PROOF_QUALITY);
posix_static_assertion(ui_font_quality_nonantialiased == NONANTIALIASED_QUALITY);
posix_static_assertion(ui_font_quality_antialiased == ANTIALIASED_QUALITY);
posix_static_assertion(ui_font_quality_cleartype == CLEARTYPE_QUALITY);
posix_static_assertion(ui_font_quality_cleartype_natural == CLEARTYPE_NATURAL_QUALITY);

static ui_font_t ui_draw_create_font(const char* family, int32_t h, int32_t q) {
    posix_assert(h > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(ui_app.fm.prop.normal.font, sizeof(lf), &lf);
    posix_fatal_if(n != (int32_t)sizeof(lf));
    lf.lfHeight = -h;
    posix_str_printf(lf.lfFaceName, "%s", family);
    if (ui_font_quality_default <= q &&
        q <= ui_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)q;
    } else {
        posix_fatal_if(q != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}

static ui_font_t ui_draw_font(ui_font_t f, int32_t h, int32_t q) {
    posix_assert(f != null && h > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(f, sizeof(lf), &lf);
    posix_fatal_if(n != (int32_t)sizeof(lf));
    lf.lfHeight = -h;
    if (ui_font_quality_default <= q &&
        q <= ui_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)q;
    } else {
        posix_fatal_if(q != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}

static void ui_draw_delete_font(ui_font_t f) {
    posix_fatal_win32err(DeleteFont(f));
}

// guaranteed to return dc != null even if not painting

static HDC ui_draw_get_dc(void) {
    // ui_app.window may be null in early init (font metrics before the
    // main window exists). GetDC(null) is documented to return the
    // screen DC, but on some Windows hosts (observed on mb-air-2012)
    // it returns null. Fall back to CreateICA which produces a
    // non-drawable Information Context for the display — adequate
    // for the measurement-only callers that hit this path.
    HDC hdc = ui_draw_hdc() != null ?
              ui_draw_hdc() : GetDC((HWND)ui_app.window);
    if (hdc == null && ui_app.window == null) {
        hdc = CreateICA("DISPLAY", null, null, null);
    }
    posix_not_null(hdc);
    return hdc;
}

static void ui_draw_release_dc(HDC hdc) {
    if (ui_draw_hdc() == null) {
        // ReleaseDC pairs with GetDC for window DCs but is the wrong
        // call for an Information Context produced by CreateICA. When
        // ReleaseDC fails (returns 0) we are looking at the ICA-
        // fallback path from get_dc above and DeleteDC is the correct
        // disposal.
        if (!ReleaseDC((HWND)ui_app.window, hdc)) {
            DeleteDC(hdc);
        }
    }
}

#define ui_draw_with_hdc(code) do {           \
    HDC hdc = ui_draw_get_dc();               \
    code                                     \
    ui_draw_release_dc(hdc);                  \
} while (0)

#define ui_draw_hdc_with_font(f, ...) do {    \
    posix_not_null(f);                          \
    HDC hdc = ui_draw_get_dc();               \
    HFONT font_ = SelectFont(hdc, (HFONT)f); \
    { __VA_ARGS__ }                          \
    SelectFont(hdc, font_);                  \
    ui_draw_release_dc(hdc);                  \
} while (0)

static void ui_draw_dump_hdc_fm(HDC hdc) {
    // https://en.wikipedia.org/wiki/Quad_(typography)
    // https://learn.microsoft.com/en-us/windows/win32/gdi/string-widths-and-heights
    // https://stackoverflow.com/questions/27631736/meaning-of-top-ascent-baseline-descent-bottom-and-leading-in-androids-font
    // Amazingly same since Windows 3.1 1992
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-textmetrica
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-outlinetextmetrica
    TEXTMETRICA tm = {0};
    posix_fatal_win32err(GetTextMetricsA(hdc, &tm));
    char pitch[64] = { 0 };
    if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH) { strcat(pitch, "FIXED_PITCH "); }
    if (tm.tmPitchAndFamily & TMPF_VECTOR)      { strcat(pitch, "VECTOR "); }
    if (tm.tmPitchAndFamily & TMPF_DEVICE)      { strcat(pitch, "DEVICE "); }
    if (tm.tmPitchAndFamily & TMPF_TRUETYPE)    { strcat(pitch, "TRUETYPE "); }
    posix_println("tm: .pitch_and_family: %s", pitch);
    posix_println(".height            : %2d   .ascent (baseline) : %2d  .descent: %2d",
            tm.tmHeight, tm.tmAscent, tm.tmDescent);
    posix_println(".internal_leading  : %2d   .external_leading  : %2d  .ave_char_width: %2d",
            tm.tmInternalLeading, tm.tmExternalLeading, tm.tmAveCharWidth);
    posix_println(".max_char_width    : %2d   .weight            : %2d .overhang: %2d",
            tm.tmMaxCharWidth, tm.tmWeight, tm.tmOverhang);
    posix_println(".digitized_aspect_x: %2d   .digitized_aspect_y: %2d",
            tm.tmDigitizedAspectX, tm.tmDigitizedAspectY);
    posix_swear(tm.tmPitchAndFamily & TMPF_TRUETYPE);
    OUTLINETEXTMETRICA otm = { .otmSize = sizeof(OUTLINETEXTMETRICA) };
    uint32_t bytes = GetOutlineTextMetricsA(hdc, otm.otmSize, &otm);
    posix_swear(bytes == sizeof(OUTLINETEXTMETRICA));
    // unsupported XHeight CapEmHeight
    // ignored:    MacDescent, MacLineGap, EMSquare, ItalicAngle
    //             CharSlopeRise, CharSlopeRun, ItalicAngle
    posix_println("otm: .Ascent       : %2d   .Descent        : %2d",
            otm.otmAscent, otm.otmDescent);
    posix_println(".otmLineGap        : %2u", otm.otmLineGap);
    posix_println(".FontBox.ltrb      :  %d,%d %2d,%2d",
            otm.otmrcFontBox.left, otm.otmrcFontBox.top,
            otm.otmrcFontBox.right, otm.otmrcFontBox.bottom);
    posix_println(".MinimumPPEM       : %2u    (minimum height in pixels)",
            otm.otmusMinimumPPEM);
    posix_println(".SubscriptOffset   : %d,%d  .SubscriptSize.x   : %dx%d",
            otm.otmptSubscriptOffset.x, otm.otmptSubscriptOffset.y,
            otm.otmptSubscriptSize.x, otm.otmptSubscriptSize.y);
    posix_println(".SuperscriptOffset : %d,%d  .SuperscriptSize.x : %dx%d",
            otm.otmptSuperscriptOffset.x, otm.otmptSuperscriptOffset.y,
            otm.otmptSuperscriptSize.x,   otm.otmptSuperscriptSize.y);
    posix_println(".UnderscoreSize    : %2d   .UnderscorePosition: %2d",
            otm.otmsUnderscoreSize, otm.otmsUnderscorePosition);
    posix_println(".StrikeoutSize     : %2u   .StrikeoutPosition : %2d ",
            otm.otmsStrikeoutSize,  otm.otmsStrikeoutPosition);
    int32_t h = otm.otmAscent + abs(tm.tmDescent); // without diacritical space above
    fp32_t pts = (h * 72.0f)  / GetDeviceCaps(hdc, LOGPIXELSY);
    posix_println("height: %.1fpt", pts);
}

static void ui_draw_dump_fm(ui_font_t f) {
    posix_not_null(f);
    ui_draw_hdc_with_font(f, { ui_draw_dump_hdc_fm(hdc); });
}

static void ui_draw_get_fm(HDC hdc, struct ui_fm* fm) {
    TEXTMETRICA tm = {0};
    posix_fatal_win32err(GetTextMetricsA(hdc, &tm));
    posix_swear(tm.tmPitchAndFamily & TMPF_TRUETYPE);
    OUTLINETEXTMETRICA otm = { .otmSize = sizeof(OUTLINETEXTMETRICA) };
    uint32_t bytes = GetOutlineTextMetricsA(hdc, otm.otmSize, &otm);
    posix_swear(bytes == sizeof(OUTLINETEXTMETRICA));
    // "tm.tmAscent" The ascent (units above the base line) of characters
    // and actually is "baseline" in other terminology
    // "otm.otmAscent" The maximum distance characters in this font extend
    // above the base line. This is the typographic ascent for the font.
    // otm.otmEMSquare usually is 2048 which is size of rasterizer
    fm->height   = tm.tmHeight;
    fm->baseline = tm.tmAscent;
    fm->ascent   = otm.otmAscent;
    fm->descent  = tm.tmDescent;
    fm->baseline = tm.tmAscent;
    fm->x_height = otm.otmsXHeight;
    fm->cap_em_height = otm.otmsCapEmHeight;
    fm->internal_leading = tm.tmInternalLeading;
    fm->external_leading = tm.tmExternalLeading;
    fm->average_char_width = tm.tmAveCharWidth;
    fm->max_char_width = tm.tmMaxCharWidth;
    fm->line_gap = otm.otmLineGap;
    fm->subscript.w = otm.otmptSubscriptSize.x;
    fm->subscript.h = otm.otmptSubscriptSize.y;
    fm->subscript_offset.x = otm.otmptSubscriptOffset.x;
    fm->subscript_offset.y = otm.otmptSubscriptOffset.y;
    fm->superscript.w = otm.otmptSuperscriptSize.x;
    fm->superscript.h = otm.otmptSuperscriptSize.y;
    fm->superscript_offset.x = otm.otmptSuperscriptOffset.x;
    fm->superscript_offset.y = otm.otmptSuperscriptOffset.y;
    fm->underscore = otm.otmsUnderscoreSize;
    fm->underscore_position = otm.otmsUnderscorePosition;
    fm->strike_through = otm.otmsStrikeoutSize;
    fm->strike_through_position = otm.otmsStrikeoutPosition;
    fm->design_units_per_em = (int)otm.otmEMSquare;
    fm->box = (struct ui_rect){
                otm.otmrcFontBox.left, otm.otmrcFontBox.top,
                otm.otmrcFontBox.right - otm.otmrcFontBox.left,
                otm.otmrcFontBox.top - otm.otmrcFontBox.bottom // inverted
    };
    // otm.Descent: The maximum distance characters in this font extend below
    // the base line. This is the typographic descent for the font.
    // Negative from the bottom (font.height)
    // tm.Descent: The descent (units below the base line) of characters.
    // Positive from the baseline down
    posix_assert(tm.tmDescent >= 0 && otm.otmDescent <= 0 &&
           -otm.otmDescent <= tm.tmDescent,
           "tm.tmDescent: %d otm.otmDescent: %d", tm.tmDescent, otm.otmDescent);
    // "Mac" typography is ignored because it's usefulness is unclear.
    // Italic angle/slant/run is ignored because at the moment edit
    // view implementation does not support italics and thus does not
    // need it. Easy to add if necessary.
}

static void ui_draw_update_fm(struct ui_fm* fm, ui_font_t f) {
    posix_not_null(f);
    SIZE em = {0, 0}; // "m"
    *fm = (struct ui_fm){ .font = f };
//  ui_draw.dump_fm(f);
    ui_draw_hdc_with_font(f, {
        ui_draw_get_fm(hdc, fm);
        // ui_glyph_nbsp and "M" have the same result
        posix_fatal_win32err(GetTextExtentPoint32A(hdc, "m", 1, &em));
        SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
        posix_fatal_win32err(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        SIZE e3 = {0}; // Three-Em Dash
        posix_fatal_win32err(GetTextExtentPoint32A(hdc,
            ui_glyph_three_em_dash, 1, &e3));
        fm->mono = em.cx == vl.cx && vl.cx == e3.cx;
//      posix_println("vl: %d %d", vl.cx, vl.cy);
//      posix_println("e3: %d %d", e3.cx, e3.cy);
//      posix_println("fm->mono: %d height: %d baseline: %d ascent: %d descent: %d",
//              fm->mono, fm->height, fm->baseline, fm->ascent, fm->descent);
    });
    posix_assert(fm->baseline <= fm->height);
    fm->em = (struct ui_wh){ .w = fm->height, .h = fm->height };
//  posix_println("fm.em: %dx%d", fm->em.w, fm->em.h);
}

static int32_t ui_draw_draw_utf16(ui_font_t font, const char* s, int32_t n,
        RECT* r, uint32_t format) { // ~70 microsecond Core i-7 3667U 2.0 GHz (2012)
    // if font == null, draws on HDC with selected font
if (0) {
    HDC hdc = ui_draw_hdc();
    if (hdc != null) {
        SIZE em = {0, 0}; // "M"
        posix_fatal_win32err(GetTextExtentPoint32A(hdc, "M", 1, &em));
        posix_println("em: %d %d", em.cx, em.cy);
        posix_fatal_win32err(GetTextExtentPoint32A(hdc, ui_glyph_em_quad, 1, &em));
        posix_println("em: %d %d", em.cx, em.cy);
        SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
        SIZE e3 = {0}; // Three-Em Dash
        posix_fatal_win32err(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        posix_println("vl: %d %d", vl.cx, vl.cy);
        posix_fatal_win32err(GetTextExtentPoint32A(hdc, ui_glyph_three_em_dash, 1, &e3));
        posix_println("e3: %d %d", e3.cx, e3.cy);
    }
}
    int32_t count = posix_str.utf16_chars(s, -1);
    posix_assert(0 < count && count < 4096, "be reasonable count: %d?", count);
    uint16_t ws[4096];
    posix_swear(count <= posix_countof(ws), "find another way to draw!");
    posix_str.utf8to16(ws, count, s, -1);
    int32_t h = 0; // return value is the height of the text
    if (font != null) {
        ui_draw_hdc_with_font(font, { h = DrawTextW(hdc, ws, n, r, format); });
    } else { // with already selected font
        ui_draw_with_hdc({ h = DrawTextW(hdc, ws, n, r, format); });
    }
    return h;
}

struct ui_draw_dtp { // draw text parameters
    const struct ui_fm* fm;
    ui_color_t color; // resolved text color (dxd draws with it)
    const char* format; // format string
    va_list va;
    RECT rc;
    uint32_t flags; // flags:
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-drawtextw
    // DT_CALCRECT DT_NOCLIP useful for measure
    // DT_END_ELLIPSIS useful for clipping
    // DT_LEFT, DT_RIGHT, DT_CENTER useful for paragraphs
    // DT_WORDBREAK is not good (GDI does not break nicely)
    // DT_BOTTOM, DT_VCENTER limited usability in weird cases (layout is better)
    // DT_NOPREFIX not to draw underline at "&Keyboard shortcuts
    // DT_SINGLELINE versus multiline
};

static void ui_draw_text_draw(struct ui_draw_dtp* p) {
    posix_not_null(p);
    char text[4096]; // expected to be enough for single text draw
    text[0] = 0;
    posix_str.format_va(text, posix_countof(text), p->format, p->va);
    text[posix_countof(text) - 1] = 0;
    int32_t k = (int32_t)posix_str.len(text);
    if (k > 0) {
        posix_swear(k > 0 && k < posix_countof(text), "k=%d n=%d fmt=%s", k, p->format);
        const bool measure_only = (p->flags & DT_CALCRECT) != 0;
        const bool multiline = (p->flags & DT_SINGLELINE) == 0;
        const bool mnemonic = (p->flags & DT_NOPREFIX) == 0;
        const int32_t w = p->rc.right - p->rc.left;
        struct ui_wh wh = dxd_text(ui_draw_context.dxd, p->fm->font,
                              p->rc.left, p->rc.top, w, p->color,
                              text, k, measure_only, multiline, mnemonic);
        p->rc.right = p->rc.left + wh.w;
        p->rc.bottom = p->rc.top + wh.h;
    } else {
        p->rc.right = p->rc.left;
        p->rc.bottom = p->rc.top + p->fm->height;
    }
}

enum {
    sl_draw          = DT_LEFT|DT_NOCLIP|DT_SINGLELINE|DT_NOCLIP,
    sl_measure       = sl_draw|DT_CALCRECT,
    ml_draw_break    = DT_LEFT|DT_NOPREFIX|DT_NOCLIP|DT_NOFULLWIDTHCHARBREAK|
                       DT_WORDBREAK,
    ml_measure_break = ml_draw_break|DT_CALCRECT,
    ml_draw          = DT_LEFT|DT_NOPREFIX|DT_NOCLIP|DT_NOFULLWIDTHCHARBREAK,
    ml_measure       = ml_draw|DT_CALCRECT
};

static struct ui_wh ui_draw_text_with_flags(const struct ui_ta* ta,
        int32_t x, int32_t y, int32_t w,
        const char* format, va_list va, uint32_t flags) {
    const int32_t right = w == 0 ? 0 : x + w;
    ui_color_t c = ui_colors.transparent; // unused when measuring
    if (!ta->measure) {
        c = ta->color;
        if (ui_color_is_undefined(c)) {
            posix_swear(ta->color_id > 0);
            c = ui_colors.get_color(ta->color_id);
        } else {
            posix_swear(ta->color_id == 0);
        }
    }
    struct ui_draw_dtp p = {
        .fm = ta->fm,
        .color = c,
        .format = format,
        .va = va,
        .rc = {.left = x, .top = y, .right = right, .bottom = 0 },
        .flags = flags
    };
    ui_draw_text_draw(&p);
    return (struct ui_wh){ p.rc.right - p.rc.left, p.rc.bottom - p.rc.top };
}

static struct ui_wh ui_draw_text_va(const struct ui_ta* ta,
        int32_t x, int32_t y,  const char* format, va_list va) {
    const uint32_t flags = sl_draw | (ta->measure ? sl_measure : 0);
    return ui_draw_text_with_flags(ta, x, y, 0, format, va, flags);
}

static struct ui_wh ui_draw_text(const struct ui_ta* ta,
        int32_t x, int32_t y, const char* format, ...) {
    const uint32_t flags = sl_draw | (ta->measure ? sl_measure : 0);
    va_list va;
    va_start(va, format);
    struct ui_wh wh = ui_draw_text_with_flags(ta, x, y, 0, format, va, flags);
    va_end(va);
    return wh;
}

static struct ui_wh ui_draw_multiline_va(const struct ui_ta* ta,
        int32_t x, int32_t y, int32_t w, const char* format, va_list va) {
    const uint32_t flags = ta->measure ?
                            (w <= 0 ? ml_measure : ml_measure_break) :
                            (w <= 0 ? ml_draw    : ml_draw_break);
    return ui_draw_text_with_flags(ta, x, y, w, format, va, flags);
}

static struct ui_wh ui_draw_multiline(const struct ui_ta* ta,
        int32_t x, int32_t y, int32_t w, const char* format, ...) {
    va_list va;
    va_start(va, format);
    struct ui_wh wh = ui_draw_multiline_va(ta, x, y, w, format, va);
    va_end(va);
    return wh;
}

static struct ui_wh ui_draw_glyphs_placement(const struct ui_ta* ta,
        const char* utf8, int32_t bytes, int32_t x[], int32_t glyphs) {
    posix_swear(bytes >= 0 && glyphs >= 0 && glyphs <= bytes);
    return dxd_glyphs_placement(ta->fm->font, utf8, bytes, x, glyphs);
}

// to enable load_bitmap() function
// 1. Add
//    curl.exe https://raw.githubusercontent.com/nothings/stb/master/stb_bitmap.h stb_bitmap.h
//    to the project precompile build step
// 2. After
//    #define ui_implementation
//    include "ui/ui.h"
//    add
//    #define STBI_ASSERT(x) assert(x)
//    #define STB_bitmap_IMPLEMENTATION
//    #include "stb_bitmap.h"

static uint8_t* ui_draw_load_bitmap(const void* data, int32_t bytes, int* w, int* h,
        int* bytes_per_pixel, int32_t preferred_bytes_per_pixel) {
    #ifdef STBI_VERSION
        return stbi_load_from_memory((uint8_t const*)data, bytes, w, h,
            bytes_per_pixel, preferred_bytes_per_pixel);
    #else // see instructions above
        (void)data; (void)bytes; (void)data; (void)w; (void)h;
        (void)bytes_per_pixel; (void)preferred_bytes_per_pixel;
        posix_fatal_if(true, "curl.exe --silent --fail --create-dirs "
            "https://raw.githubusercontent.com/nothings/stb/master/stb_bitmap.h "
            "--output ext/stb_bitmap.h");
        return null;
    #endif
}

static void ui_draw_bitmap_dispose(struct ui_bitmap* image) {
    dxd_bitmap_dispose(&image->dxd);
    posix_fatal_win32err(DeleteBitmap(image->texture));
    memset(image, 0, sizeof(struct ui_bitmap));
}

struct ui_draw_if ui_draw = {
    .ta = {
        .prop = {
            .normal = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.prop.normal,
                .measure  = false
            },
            .title = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.prop.title,
                .measure  = false
            },
            .rubric = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.prop.rubric,
                .measure  = false
            },
            .H1 = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.prop.H1,
                .measure  = false
            },
            .H2 = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.prop.H2,
                .measure  = false
            },
            .H3 = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.prop.H3,
                .measure  = false
            }
        },
        .mono = {
            .normal = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.mono.normal,
                .measure  = false
            },
            .title = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.mono.title,
                .measure  = false
            },
            .rubric = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.mono.rubric,
                .measure  = false
            },
            .H1 = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.mono.H1,
                .measure  = false
            },
            .H2 = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.mono.H2,
                .measure  = false
            },
            .H3 = {
                .color_id = ui_color_id_window_text,
                .color    = ui_color_undefined,
                .fm       = &ui_app.fm.mono.H3,
                .measure  = false
            }
        },
    },
    .init                     = ui_draw_init,
    .begin                    = ui_draw_begin,
    .end                      = ui_draw_end,
    .color_rgb                = ui_draw_color_rgb,
    .bitmap_init              = ui_draw_bitmap_init,
    .bitmap_init_rgbx         = ui_draw_bitmap_init_rgbx,
    .bitmap_dispose           = ui_draw_bitmap_dispose,
    .alpha                    = ui_draw_alpha,
    .bitmap                   = ui_draw_bitmap,
    .icon                     = ui_draw_icon,
    .set_clip                 = ui_draw_set_clip,
    .pixel                    = ui_draw_pixel,
    .line                     = ui_draw_line,
    .frame                    = ui_draw_frame,
    .rect                     = ui_draw_rect,
    .fill                     = ui_draw_fill,
    .poly                     = ui_draw_poly,
    .circle                   = ui_draw_circle,
    .rounded                  = ui_draw_rounded,
    .gradient                 = ui_draw_gradient,
    .pixels                   = ui_draw_pixels,
    .greyscale                = ui_draw_greyscale,
    .bgr                      = ui_draw_bgr,
    .bgrx                     = ui_draw_bgrx,
    .cleartype                = ui_draw_cleartype,
    .font_smoothing_contrast  = ui_draw_font_smoothing_contrast,
    .create_font              = ui_draw_create_font,
    .font                     = ui_draw_font,
    .delete_font              = ui_draw_delete_font,
    .dump_fm                  = ui_draw_dump_fm,
    .update_fm                = ui_draw_update_fm,
    .text_va                  = ui_draw_text_va,
    .text                     = ui_draw_text,
    .multiline_va             = ui_draw_multiline_va,
    .multiline                = ui_draw_multiline,
    .glyphs_placement         = ui_draw_glyphs_placement,
    .fini                     = ui_draw_fini
};

#pragma pop_macro("ui_draw_hdc_with_font")
#pragma pop_macro("ui_draw_with_hdc")
