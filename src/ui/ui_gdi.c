#include "ut/ut.h"
#include "ui/ui.h"
#include "ut/ut_win32.h"

#pragma push_macro("app_window")
#pragma push_macro("app_canvas")
#pragma push_macro("gdi_with_hdc")
#pragma push_macro("gdi_hdc_with_font")

#define app_window() ((HWND)app.window)
#define app_canvas() ((HDC)app.canvas)

typedef struct ui_gdi_xyc_s {
    int32_t x;
    int32_t y;
    ui_color_t c;
} ui_gdi_xyc_t;

static int32_t ui_gdi_top;
static ui_gdi_xyc_t ui_gdi_stack[256];

static void ui_gdi_init(void) {
    ui_gdi.brush_hollow = (ui_brush_t)GetStockBrush(HOLLOW_BRUSH);
    ui_gdi.brush_color  = (ui_brush_t)GetStockBrush(DC_BRUSH);
    ui_gdi.pen_hollow = (ui_pen_t)GetStockPen(NULL_PEN);
}

static uint32_t ui_gdi_color_rgb(ui_color_t c) {
    assert(ui_color_is_8bit(c));
    return (COLORREF)(c & 0xFFFFFFFF);
}

static COLORREF ui_gdi_color_ref(ui_color_t c) {
    return ui_gdi.color_rgb(c);
}

static ui_color_t ui_gdi_set_text_color(ui_color_t c) {
    return SetTextColor(app_canvas(), ui_gdi_color_ref(c));
}

static ui_pen_t ui_gdi_set_pen(ui_pen_t p) {
    not_null(p);
    return (ui_pen_t)SelectPen(app_canvas(), (HPEN)p);
}

static ui_pen_t ui_gdi_set_colored_pen(ui_color_t c) {
    ui_pen_t p = (ui_pen_t)SelectPen(app_canvas(), GetStockPen(DC_PEN));
    SetDCPenColor(app_canvas(), ui_gdi_color_ref(c));
    return p;
}

static ui_pen_t ui_gdi_create_pen(ui_color_t c, int32_t width) {
    assert(width >= 1);
    ui_pen_t pen = (ui_pen_t)CreatePen(PS_SOLID, width, ui_gdi_color_ref(c));
    not_null(pen);
    return pen;
}

static void ui_gdi_delete_pen(ui_pen_t p) {
    fatal_if_false(DeletePen(p));
}

static ui_brush_t ui_gdi_create_brush(ui_color_t c) {
    return (ui_brush_t)CreateSolidBrush(ui_gdi_color_ref(c));
}

static void ui_gdi_delete_brush(ui_brush_t b) {
    DeleteBrush((HBRUSH)b);
}

static ui_brush_t ui_gdi_set_brush(ui_brush_t b) {
    not_null(b);
    return (ui_brush_t)SelectBrush(app_canvas(), b);
}

static ui_color_t ui_gdi_set_brush_color(ui_color_t c) {
    return SetDCBrushColor(app_canvas(), ui_gdi_color_ref(c));
}

static void ui_gdi_set_clip(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (ui_gdi.clip != null) { DeleteRgn(ui_gdi.clip); ui_gdi.clip = null; }
    if (w > 0 && h > 0) {
        ui_gdi.clip = (ui_region_t)CreateRectRgn(x, y, x + w, y + h);
        not_null(ui_gdi.clip);
    }
    fatal_if(SelectClipRgn(app_canvas(), (HRGN)ui_gdi.clip) == ERROR);
}

static void ui_gdi_push(int32_t x, int32_t y) {
    assert(ui_gdi_top < countof(ui_gdi_stack));
    fatal_if(ui_gdi_top >= countof(ui_gdi_stack));
    ui_gdi_stack[ui_gdi_top].x = ui_gdi.x;
    ui_gdi_stack[ui_gdi_top].y = ui_gdi.y;
    fatal_if(SaveDC(app_canvas()) == 0);
    ui_gdi_top++;
    ui_gdi.x = x;
    ui_gdi.y = y;
}

static void ui_gdi_pop(void) {
    assert(0 < ui_gdi_top && ui_gdi_top <= countof(ui_gdi_stack));
    fatal_if(ui_gdi_top <= 0);
    ui_gdi_top--;
    ui_gdi.x = ui_gdi_stack[ui_gdi_top].x;
    ui_gdi.y = ui_gdi_stack[ui_gdi_top].y;
    fatal_if_false(RestoreDC(app_canvas(), -1));
}

static void ui_gdi_pixel(int32_t x, int32_t y, ui_color_t c) {
    not_null(app.canvas);
    fatal_if_false(SetPixel(app_canvas(), x, y, ui_gdi_color_ref(c)));
}

static ui_point_t ui_gdi_move_to(int32_t x, int32_t y) {
    POINT pt;
    pt.x = ui_gdi.x;
    pt.y = ui_gdi.y;
    fatal_if_false(MoveToEx(app_canvas(), x, y, &pt));
    ui_gdi.x = x;
    ui_gdi.y = y;
    ui_point_t p = { pt.x, pt.y };
    return p;
}

static void ui_gdi_line(int32_t x, int32_t y) {
    fatal_if_false(LineTo(app_canvas(), x, y));
    ui_gdi.x = x;
    ui_gdi.y = y;
}

static void ui_gdi_frame(int32_t x, int32_t y, int32_t w, int32_t h) {
    ui_brush_t b = ui_gdi.set_brush(ui_gdi.brush_hollow);
    ui_gdi.rect(x, y, w, h);
    ui_gdi.set_brush(b);
}

static void ui_gdi_rect(int32_t x, int32_t y, int32_t w, int32_t h) {
    fatal_if_false(Rectangle(app_canvas(), x, y, x + w, y + h));
}

static void ui_gdi_fill(int32_t x, int32_t y, int32_t w, int32_t h) {
    RECT rc = { x, y, x + w, y + h };
    ui_brush_t b = (ui_brush_t)GetCurrentObject(app_canvas(), OBJ_BRUSH);
    fatal_if_false(FillRect(app_canvas(), &rc, (HBRUSH)b));
}

static void ui_gdi_frame_with(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
    ui_brush_t b = ui_gdi.set_brush(ui_gdi.brush_hollow);
    ui_pen_t p = ui_gdi.set_colored_pen(c);
    ui_gdi.rect(x, y, w, h);
    ui_gdi.set_pen(p);
    ui_gdi.set_brush(b);
}

static void ui_gdi_rect_with(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t border, ui_color_t fill) {
    ui_brush_t b = ui_gdi.set_brush(ui_gdi.brush_color);
    ui_color_t c = ui_gdi.set_brush_color(fill);
    ui_pen_t p = ui_gdi.set_colored_pen(border);
    ui_gdi.rect(x, y, w, h);
    ui_gdi.set_brush_color(c);
    ui_gdi.set_pen(p);
    ui_gdi.set_brush(b);
}

static void ui_gdi_fill_with(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
    ui_brush_t b = ui_gdi.set_brush(ui_gdi.brush_color);
    c = ui_gdi.set_brush_color(c);
    ui_gdi.fill(x, y, w, h);
    ui_gdi.set_brush_color(c);
    ui_gdi.set_brush(b);
}

static void ui_gdi_poly(ui_point_t* points, int32_t count) {
    // make sure ui_point_t and POINT have the same memory layout:
    static_assert(sizeof(points->x) == sizeof(((POINT*)0)->x), "ui_point_t");
    static_assert(sizeof(points->y) == sizeof(((POINT*)0)->y), "ui_point_t");
    static_assert(sizeof(points[0]) == sizeof(*((POINT*)0)), "ui_point_t");
    assert(app_canvas() != null && count > 1);
    fatal_if_false(Polyline(app_canvas(), (POINT*)points, count));
}

static void ui_gdi_rounded(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t rx, int32_t ry) {
    fatal_if_false(RoundRect(app_canvas(), x, y, x + w, y + h, rx, ry));
}

static void ui_gdi_gradient(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t rgba_from, ui_color_t rgba_to, bool vertical) {
    TRIVERTEX vertex[2];
    vertex[0].x = x;
    vertex[0].y = y;
    // TODO: colors:
    vertex[0].Red   = ((rgba_from >>  0) & 0xFF) << 8;
    vertex[0].Green = ((rgba_from >>  8) & 0xFF) << 8;
    vertex[0].Blue  = ((rgba_from >> 16) & 0xFF) << 8;
    vertex[0].Alpha = ((rgba_from >> 24) & 0xFF) << 8;
    vertex[1].x = x + w;
    vertex[1].y = y + h;
    vertex[1].Red   = ((rgba_to >>  0) & 0xFF) << 8;
    vertex[1].Green = ((rgba_to >>  8) & 0xFF) << 8;
    vertex[1].Blue  = ((rgba_to >> 16) & 0xFF) << 8;
    vertex[1].Alpha = ((rgba_to >> 24) & 0xFF) << 8;
    GRADIENT_RECT gRect = {0, 1};
    const int32_t mode = vertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H;
    GradientFill(app_canvas(), vertex, 2, &gRect, 1, mode);
}

static BITMAPINFO* ui_gdi_greyscale_bitmap_info(void) {
    typedef struct bitmap_rgb_s {
        BITMAPINFO bi;
        RGBQUAD rgb[256];
    } bitmap_rgb_t;
    static bitmap_rgb_t storage; // for grayscale palette
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

static void ui_gdi_draw_greyscale(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels) {
    fatal_if(stride != ((iw + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFO *bi = ui_gdi_greyscale_bitmap_info(); // global! not thread safe
        BITMAPINFOHEADER* bih = &bi->bmiHeader;
        bih->biWidth = iw;
        bih->biHeight = -ih; // top down image
        bih->biSizeImage = w * h;
        POINT pt = { 0 };
        fatal_if_false(SetBrushOrgEx(app_canvas(), 0, 0, &pt));
        fatal_if(StretchDIBits(app_canvas(), sx, sy, sw, sh, x, y, w, h,
            pixels, bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        fatal_if_false(SetBrushOrgEx(app_canvas(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFOHEADER ui_gdi_bgrx_init_bi(int32_t w, int32_t h, int32_t bpp) {
    BITMAPINFOHEADER bi = {
        .biSize = sizeof(BITMAPINFOHEADER),
        .biPlanes = 1,
        .biBitCount = (uint16_t)(bpp * 8),
        .biCompression = BI_RGB,
        .biWidth = w,
        .biHeight = -h, // top down image
        .biSizeImage = w * h * bpp,
        .biClrUsed = 0,
        .biClrImportant = 0
   };
   return bi;
}

// draw_bgr(iw) assumes strides are padded and rounded up to 4 bytes
// if this is not the case use ui_gdi.image_init() that will unpack
// and align scanlines prior to draw

static void ui_gdi_draw_bgr(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride,
        const uint8_t* pixels) {
    fatal_if(stride != ((iw * 3 + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFOHEADER bi = ui_gdi_bgrx_init_bi(iw, ih, 3);
        POINT pt = { 0 };
        fatal_if_false(SetBrushOrgEx(app_canvas(), 0, 0, &pt));
        fatal_if(StretchDIBits(app_canvas(), sx, sy, sw, sh, x, y, w, h,
            pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        fatal_if_false(SetBrushOrgEx(app_canvas(), pt.x, pt.y, &pt));
    }
}

static void ui_gdi_draw_bgrx(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride,
        const uint8_t* pixels) {
    fatal_if(stride != ((iw * 4 + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFOHEADER bi = ui_gdi_bgrx_init_bi(iw, ih, 4);
        POINT pt = { 0 };
        fatal_if_false(SetBrushOrgEx(app_canvas(), 0, 0, &pt));
        fatal_if(StretchDIBits(app_canvas(), sx, sy, sw, sh, x, y, w, h,
            pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        fatal_if_false(SetBrushOrgEx(app_canvas(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFO* ui_gdi_init_bitmap_info(int32_t w, int32_t h, int32_t bpp,
        BITMAPINFO* bi) {
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = w;
    bi->bmiHeader.biHeight = -h;  // top down image
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = (uint16_t)(bpp * 8);
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biSizeImage = w * h * bpp;
    return bi;
}

static void ui_gdi_create_dib_section(ui_image_t* image, int32_t w, int32_t h,
        int32_t bpp) {
    fatal_if(image->bitmap != null, "image_dispose() not called?");
    // not using GetWindowDC(app_window()) will allow to initialize images
    // before window is created
    HDC c = CreateCompatibleDC(null); // GetWindowDC(app_window());
    BITMAPINFO local = { {sizeof(BITMAPINFOHEADER)} };
    BITMAPINFO* bi = bpp == 1 ? ui_gdi_greyscale_bitmap_info() : &local;
    image->bitmap = (ui_bitmap_t)CreateDIBSection(c, ui_gdi_init_bitmap_info(w, h, bpp, bi),
                                               DIB_RGB_COLORS, &image->pixels, null, 0x0);
    fatal_if(image->bitmap == null || image->pixels == null);
//  fatal_if_false(ReleaseDC(app_window(), c));
    fatal_if_false(DeleteDC(c));
}

static void ui_gdi_image_init_rgbx(ui_image_t* image, int32_t w, int32_t h,
        int32_t bpp, const uint8_t* pixels) {
    bool swapped = bpp < 0;
    bpp = abs(bpp);
    fatal_if(bpp != 4, "bpp: %d", bpp);
    ui_gdi_create_dib_section(image, w, h, bpp);
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

static void ui_gdi_image_init(ui_image_t* image, int32_t w, int32_t h, int32_t bpp,
        const uint8_t* pixels) {
    bool swapped = bpp < 0;
    bpp = abs(bpp);
    fatal_if(bpp < 0 || bpp == 2 || bpp > 4, "bpp=%d not {1, 3, 4}", bpp);
    ui_gdi_create_dib_section(image, w, h, bpp);
    // Win32 bitmaps stride is rounded up to 4 bytes
    const int32_t stride = (w * bpp + 3) & ~0x3;
    uint8_t* scanline = image->pixels;
    if (bpp == 1) {
        for (int32_t y = 0; y < h; y++) {
            memcpy(scanline, pixels, w);
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

static void ui_gdi_alpha_blend(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image, fp64_t alpha) {
    assert(image->bpp > 0);
    assert(0 <= alpha && alpha <= 1);
    not_null(app_canvas());
    HDC c = CreateCompatibleDC(app_canvas());
    not_null(c);
    HBITMAP zero1x1 = SelectBitmap((HDC)c, (HBITMAP)image->bitmap);
    BLENDFUNCTION bf = { 0 };
    bf.SourceConstantAlpha = (uint8_t)(0xFF * alpha + 0.49);
    if (image->bpp == 4) {
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.AlphaFormat = AC_SRC_ALPHA;
    } else {
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.AlphaFormat = 0;
    }
    fatal_if_false(AlphaBlend(app_canvas(), x, y, w, h,
        c, 0, 0, image->w, image->h, bf));
    SelectBitmap((HDC)c, zero1x1);
    fatal_if_false(DeleteDC(c));
}

static void ui_gdi_draw_image(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image) {
    assert(image->bpp == 1 || image->bpp == 3 || image->bpp == 4);
    not_null(app_canvas());
    if (image->bpp == 1) { // StretchBlt() is bad for greyscale
        BITMAPINFO* bi = ui_gdi_greyscale_bitmap_info();
        fatal_if(StretchDIBits(app_canvas(), x, y, w, h, 0, 0, image->w, image->h,
            image->pixels, ui_gdi_init_bitmap_info(image->w, image->h, 1, bi),
            DIB_RGB_COLORS, SRCCOPY) == 0);
    } else {
        HDC c = CreateCompatibleDC(app_canvas());
        not_null(c);
        HBITMAP zero1x1 = SelectBitmap(c, image->bitmap);
        fatal_if_false(StretchBlt(app_canvas(), x, y, w, h,
            c, 0, 0, image->w, image->h, SRCCOPY));
        SelectBitmap(c, zero1x1);
        fatal_if_false(DeleteDC(c));
    }
}

static void ui_gdi_draw_icon(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_icon_t icon) {
    DrawIconEx(app_canvas(), x, y, (HICON)icon, w, h, 0, NULL, DI_NORMAL | DI_COMPAT);
}

static void ui_gdi_cleartype(bool on) {
    enum { spif = SPIF_UPDATEINIFILE | SPIF_SENDCHANGE };
    fatal_if_false(SystemParametersInfoA(SPI_SETFONTSMOOTHING, true, 0, spif));
    uintptr_t s = on ? FE_FONTSMOOTHINGCLEARTYPE : FE_FONTSMOOTHINGSTANDARD;
    fatal_if_false(SystemParametersInfoA(SPI_SETFONTSMOOTHINGTYPE, 0,
        (void*)s, spif));
}

static void ui_gdi_font_smoothing_contrast(int32_t c) {
    fatal_if(!(c == -1 || 1000 <= c && c <= 2200), "contrast: %d", c);
    if (c == -1) { c = 1400; }
    fatal_if_false(SystemParametersInfoA(SPI_SETFONTSMOOTHINGCONTRAST, 0,
                   (void*)(uintptr_t)c, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE));
}

static_assertion(ui_gdi_font_quality_default == DEFAULT_QUALITY);
static_assertion(ui_gdi_font_quality_draft == DRAFT_QUALITY);
static_assertion(ui_gdi_font_quality_proof == PROOF_QUALITY);
static_assertion(ui_gdi_font_quality_nonantialiased == NONANTIALIASED_QUALITY);
static_assertion(ui_gdi_font_quality_antialiased == ANTIALIASED_QUALITY);
static_assertion(ui_gdi_font_quality_cleartype == CLEARTYPE_QUALITY);
static_assertion(ui_gdi_font_quality_cleartype_natural == CLEARTYPE_NATURAL_QUALITY);

static ui_font_t ui_gdi_create_font(const char* family, int32_t height, int32_t quality) {
    assert(height > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(app.fonts.regular, sizeof(lf), &lf);
    fatal_if_false(n == (int32_t)sizeof(lf));
    lf.lfHeight = -height;
    strprintf(lf.lfFaceName, "%s", family);
    if (ui_gdi_font_quality_default <= quality && quality <= ui_gdi_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)quality;
    } else {
        fatal_if(quality != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}


static ui_font_t ui_gdi_font(ui_font_t f, int32_t height, int32_t quality) {
    assert(f != null && height > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(f, sizeof(lf), &lf);
    fatal_if_false(n == (int32_t)sizeof(lf));
    lf.lfHeight = -height;
    if (ui_gdi_font_quality_default <= quality && quality <= ui_gdi_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)quality;
    } else {
        fatal_if(quality != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}

static int32_t ui_gdi_font_height(ui_font_t f) {
    assert(f != null);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(f, sizeof(lf), &lf);
    fatal_if_false(n == (int32_t)sizeof(lf));
    assert(lf.lfHeight < 0);
    return abs(lf.lfHeight);
}

static void ui_gdi_delete_font(ui_font_t f) {
    fatal_if_false(DeleteFont(f));
}

static ui_font_t ui_gdi_set_font(ui_font_t f) {
    not_null(f);
    return (ui_font_t)SelectFont(app_canvas(), (HFONT)f);
}

#define ui_gdi_with_hdc(code) do {                                          \
    not_null(app_window());                                              \
    HDC hdc = app_canvas() != null ? app_canvas() : GetDC(app_window()); \
    not_null(hdc);                                                       \
    code                                                                 \
    if (app_canvas() == null) {                                          \
        ReleaseDC(app_window(), hdc);                                    \
    }                                                                    \
} while (0);

#define ui_gdi_hdc_with_font(f, ...) do {                                   \
    not_null(f);                                                         \
    not_null(app_window());                                              \
    HDC hdc = app_canvas() != null ? app_canvas() : GetDC(app_window()); \
    not_null(hdc);                                                       \
    HFONT _font_ = SelectFont(hdc, (HFONT)f);                            \
    { __VA_ARGS__ }                                                      \
    SelectFont(hdc, _font_);                                             \
    if (app_canvas() == null) {                                          \
        ReleaseDC(app_window(), hdc);                                        \
    }                                                                    \
} while (0);


static int32_t ui_gdi_baseline(ui_font_t f) {
    TEXTMETRICA tm;
    ui_gdi_hdc_with_font(f, {
        fatal_if_false(GetTextMetricsA(hdc, &tm));
    })
    return tm.tmAscent;
}

static int32_t ui_gdi_descent(ui_font_t f) {
    TEXTMETRICA tm;
    ui_gdi_hdc_with_font(f, {
        fatal_if_false(GetTextMetricsA(hdc, &tm));
    });
    return tm.tmDescent;
}

static ui_point_t ui_gdi_get_em(ui_font_t f) {
    // 24 microseconds Core i-7 3667U 2.0 GHz (2012)
    SIZE cell = {0, 0};
    int32_t height   = 0;
    int32_t descent  = 0;
    int32_t baseline = 0;
    ui_gdi_hdc_with_font(f, {
        // ui_glyph_nbsp and "M" have the same result
        fatal_if_false(GetTextExtentPoint32A(hdc, "M", 1, &cell));
        height = ui_gdi.font_height(f);
        descent = ui_gdi.descent(f);
        baseline = ui_gdi.baseline(f);
    });
    assert(baseline >= height);
    ui_point_t c = {cell.cx, cell.cy - descent - (height - baseline)};
    return c;
}

static bool ui_gdi_is_mono(ui_font_t f) {
    SIZE em = {0}; // "M"
    SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
    SIZE e3 = {0}; // "\xE2\xB8\xBB" Three-Em Dash https://www.compart.com/en/unicode/U+2E3B
    ui_gdi_hdc_with_font(f, {
        fatal_if_false(GetTextExtentPoint32A(hdc, "M", 1, &em));
        fatal_if_false(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        fatal_if_false(GetTextExtentPoint32A(hdc, "\xE2\xB8\xBB", 1, &e3));
    });
    return em.cx == vl.cx && vl.cx == e3.cx;
}

static fp64_t ui_gdi_line_spacing(fp64_t height_multiplier) {
    assert(0.1 <= height_multiplier && height_multiplier <= 2.0);
    fp64_t hm = ui_gdi.height_multiplier;
    ui_gdi.height_multiplier = height_multiplier;
    return hm;
}

static int32_t ui_gdi_draw_utf16(ui_font_t font, const char* s, int32_t n,
        RECT* r, uint32_t format) { // ~70 microsecond Core i-7 3667U 2.0 GHz (2012)
    // if font == null, draws on HDC with selected font
    int32_t height = 0; // return value is the height of the text in logical units
    if (font != null) {
        ui_gdi_hdc_with_font(font, {
            height = DrawTextW(hdc, utf8to16(s), n, r, format);
        });
    } else {
        ui_gdi_with_hdc({
            height = DrawTextW(hdc, utf8to16(s), n, r, format);
        });
    }
    return height;
}

typedef struct { // draw text parameters
    ui_font_t font;
    const char* format; // format string
    va_list vl;
    RECT rc;
    uint32_t flags; // flags:
    // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-drawtextw
    // DT_CALCRECT DT_NOCLIP useful for measure
    // DT_END_ELLIPSIS useful for clipping
    // DT_LEFT, DT_RIGHT, DT_CENTER useful for paragraphs
    // DT_WORDBREAK is not good (GDI does not break nicely)
    // DT_BOTTOM, DT_VCENTER limited usablity in wierd cases (layout is better)
    // DT_NOPREFIX not to draw underline at "&Keyboard shortcuts
    // DT_SINGLELINE versus multiline
} ui_gdi_dtp_t;

static void ui_gdi_text_draw(ui_gdi_dtp_t* p) {
    int32_t n = 1024;
    char* text = (char*)ut_stackalloc(n);
    ut_str.format_va(text, n - 1, p->format, p->vl);
    int32_t k = (int32_t)strlen(text);
    // Microsoft returns -1 not posix required sizeof buffer
    while (k >= n - 1 || k < 0) {
        n = n * 2;
        text = (char*)ut_stackalloc(n);
        ut_str.format_va(text, n - 1, p->format, p->vl);
        k = (int32_t)strlen(text);
    }
    assert(k >= 0 && k <= n, "k=%d n=%d fmt=%s", k, n, p->format);
    // rectangle is always calculated - it makes draw text
    // much slower but UI layer is mostly uses bitmap caching:
    if ((p->flags & DT_CALCRECT) == 0) {
        // no actual drawing just calculate rectangle
        bool b = ui_gdi_draw_utf16(p->font, text, -1, &p->rc, p->flags | DT_CALCRECT);
        assert(b, "draw_text_utf16(%s) failed", text); (void)b;
    }
    bool b = ui_gdi_draw_utf16(p->font, text, -1, &p->rc, p->flags);
    assert(b, "draw_text_utf16(%s) failed", text); (void)b;
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

static ui_point_t ui_gdi_text_measure(ui_gdi_dtp_t* p) {
    ui_gdi_text_draw(p);
    ui_point_t cell = {p->rc.right - p->rc.left, p->rc.bottom - p->rc.top};
    return cell;
}

static ui_point_t ui_gdi_measure_singleline(ui_font_t f, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi_dtp_t p = { f, format, vl, {0, 0, 0, 0}, sl_measure };
    ui_point_t cell = ui_gdi_text_measure(&p);
    va_end(vl);
    return cell;
}

static ui_point_t ui_gdi_measure_multiline(ui_font_t f, int32_t w, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    uint32_t flags = w <= 0 ? ml_measure : ml_measure_break;
    ui_gdi_dtp_t p = { f, format, vl, {ui_gdi.x, ui_gdi.y, ui_gdi.x + (w <= 0 ? 1 : w), ui_gdi.y}, flags };
    ui_point_t cell = ui_gdi_text_measure(&p);
    va_end(vl);
    return cell;
}

static void ui_gdi_vtext(const char* format, va_list vl) {
    ui_gdi_dtp_t p = { null, format, vl, {ui_gdi.x, ui_gdi.y, 0, 0}, sl_draw };
    ui_gdi_text_draw(&p);
    ui_gdi.x += p.rc.right - p.rc.left;
}

static void ui_gdi_vtextln(const char* format, va_list vl) {
    ui_gdi_dtp_t p = { null, format, vl, {ui_gdi.x, ui_gdi.y, ui_gdi.x, ui_gdi.y}, sl_draw };
    ui_gdi_text_draw(&p);
    ui_gdi.y += (int32_t)((p.rc.bottom - p.rc.top) * ui_gdi.height_multiplier + 0.5f);
}

static void ui_gdi_text(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi.vtext(format, vl);
    va_end(vl);
}

static void ui_gdi_textln(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi.vtextln(format, vl);
    va_end(vl);
}

static ui_point_t ui_gdi_multiline(int32_t w, const char* f, ...) {
    va_list vl;
    va_start(vl, f);
    uint32_t flags = w <= 0 ? ml_draw : ml_draw_break;
    ui_gdi_dtp_t p = { null, f, vl, {ui_gdi.x, ui_gdi.y, ui_gdi.x + (w <= 0 ? 1 : w), ui_gdi.y}, flags };
    ui_gdi_text_draw(&p);
    va_end(vl);
    ui_point_t c = { p.rc.right - p.rc.left, p.rc.bottom - p.rc.top };
    return c;
}

static void ui_gdi_vprint(const char* format, va_list vl) {
    not_null(app.fonts.mono);
    ui_font_t f = ui_gdi.set_font(app.fonts.mono);
    ui_gdi.vtext(format, vl);
    ui_gdi.set_font(f);
}

static void ui_gdi_print(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi.vprint(format, vl);
    va_end(vl);
}

static void ui_gdi_vprintln(const char* format, va_list vl) {
    not_null(app.fonts.mono);
    ui_font_t f = ui_gdi.set_font(app.fonts.mono);
    ui_gdi.vtextln(format, vl);
    ui_gdi.set_font(f);
}

static void ui_gdi_println(const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    ui_gdi.vprintln(format, vl);
    va_end(vl);
}

// to enable load_image() function
// 1. Add
//    curl.exe https://raw.githubusercontent.com/nothings/stb/master/stb_image.h stb_image.h
//    to the project precompile build step
// 2. After
//    #define quick_implementation
//    include "quick.h"
//    add
//    #define STBI_ASSERT(x) assert(x)
//    #define STB_IMAGE_IMPLEMENTATION
//    #include "stb_image.h"

static uint8_t* ui_gdi_load_image(const void* data, int32_t bytes, int* w, int* h,
        int* bytes_per_pixel, int32_t preferred_bytes_per_pixel) {
    #ifdef STBI_VERSION
        return stbi_load_from_memory((uint8_t const*)data, bytes, w, h,
            bytes_per_pixel, preferred_bytes_per_pixel);
    #else // see instructions above
        (void)data; (void)bytes; (void)data; (void)w; (void)h;
        (void)bytes_per_pixel; (void)preferred_bytes_per_pixel;
        fatal_if(true, "curl.exe --silent --fail --create-dirs "
            "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h "
            "--output ext/stb_image.h");
        return null;
    #endif
}

static void ui_gdi_image_dispose(ui_image_t* image) {
    fatal_if_false(DeleteBitmap(image->bitmap));
    memset(image, 0, sizeof(ui_image_t));
}

ui_gdi_if ui_gdi = {
    .height_multiplier             = 1.0,
    .init                          = ui_gdi_init,
    .color_rgb                     = ui_gdi_color_rgb,
    .image_init                    = ui_gdi_image_init,
    .image_init_rgbx               = ui_gdi_image_init_rgbx,
    .image_dispose                 = ui_gdi_image_dispose,
    .alpha_blend                   = ui_gdi_alpha_blend,
    .draw_image                    = ui_gdi_draw_image,
    .draw_icon                     = ui_gdi_draw_icon,
    .set_text_color                = ui_gdi_set_text_color,
    .create_brush                  = ui_gdi_create_brush,
    .delete_brush                  = ui_gdi_delete_brush,
    .set_brush                     = ui_gdi_set_brush,
    .set_brush_color               = ui_gdi_set_brush_color,
    .set_colored_pen               = ui_gdi_set_colored_pen,
    .create_pen                    = ui_gdi_create_pen,
    .set_pen                       = ui_gdi_set_pen,
    .delete_pen                    = ui_gdi_delete_pen,
    .set_clip                      = ui_gdi_set_clip,
    .push                          = ui_gdi_push,
    .pop                           = ui_gdi_pop,
    .pixel                         = ui_gdi_pixel,
    .move_to                       = ui_gdi_move_to,
    .line                          = ui_gdi_line,
    .frame                         = ui_gdi_frame,
    .rect                          = ui_gdi_rect,
    .fill                          = ui_gdi_fill,
    .frame_with                    = ui_gdi_frame_with,
    .rect_with                     = ui_gdi_rect_with,
    .fill_with                     = ui_gdi_fill_with,
    .poly                          = ui_gdi_poly,
    .rounded                       = ui_gdi_rounded,
    .gradient                      = ui_gdi_gradient,
    .draw_greyscale                = ui_gdi_draw_greyscale,
    .draw_bgr                      = ui_gdi_draw_bgr,
    .draw_bgrx                     = ui_gdi_draw_bgrx,
    .cleartype                     = ui_gdi_cleartype,
    .font_smoothing_contrast       = ui_gdi_font_smoothing_contrast,
    .create_font                   = ui_gdi_create_font,
    .font                          = ui_gdi_font,
    .delete_font                   = ui_gdi_delete_font,
    .set_font                      = ui_gdi_set_font,
    .font_height                   = ui_gdi_font_height,
    .descent                       = ui_gdi_descent,
    .baseline                      = ui_gdi_baseline,
    .is_mono                       = ui_gdi_is_mono,
    .get_em                        = ui_gdi_get_em,
    .line_spacing                  = ui_gdi_line_spacing,
    .measure_text                  = ui_gdi_measure_singleline,
    .measure_multiline             = ui_gdi_measure_multiline,
    .vtext                         = ui_gdi_vtext,
    .vtextln                       = ui_gdi_vtextln,
    .text                          = ui_gdi_text,
    .textln                        = ui_gdi_textln,
    .vprint                        = ui_gdi_vprint,
    .vprintln                      = ui_gdi_vprintln,
    .print                         = ui_gdi_print,
    .println                       = ui_gdi_println,
    .multiline                     = ui_gdi_multiline
};

#pragma pop_macro("gdi_hdc_with_font")
#pragma pop_macro("gdi_with_hdc")
#pragma pop_macro("app_canvas")
#pragma pop_macro("app_window")
