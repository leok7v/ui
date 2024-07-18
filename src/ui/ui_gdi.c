#include "ut/ut.h"
#include "ui/ui.h"
#include "ut/ut_win32.h"

#pragma push_macro("ui_gdi_with_hdc")
#pragma push_macro("ui_gdi_hdc_with_font")

static ui_brush_t  ui_gdi_brush_hollow;
static ui_brush_t  ui_gdi_brush_color;
static ui_pen_t    ui_gdi_pen_hollow;
static ui_region_t ui_gdi_clip;

typedef struct ui_gdi_context_s {
    HDC hdc; // window canvas() or memory DC
    int32_t background_mode;
    int32_t stretch_mode;
    ui_pen_t pen;
    ui_font_t font;
    ui_color_t text_color;
    POINT brush_origin;
    ui_brush_t brush;
    HBITMAP bitmap;
} ui_gdi_context_t;

static ui_gdi_context_t ui_gdi_context;

#define ui_gdi_hdc() (ui_gdi_context.hdc)

static void ui_gdi_init(void) {
    ui_gdi_brush_hollow = (ui_brush_t)GetStockBrush(HOLLOW_BRUSH);
    ui_gdi_brush_color  = (ui_brush_t)GetStockBrush(DC_BRUSH);
    ui_gdi_pen_hollow = (ui_pen_t)GetStockPen(NULL_PEN);
}

static void ui_gdi_fini(void) {
    if (ui_gdi_clip != null) {
        ut_fatal_win32err(DeleteRgn(ui_gdi_clip));
    }
    ui_gdi_clip = null;
}

static ui_pen_t ui_gdi_set_pen(ui_pen_t p) {
    ut_not_null(p);
    return (ui_pen_t)SelectPen(ui_gdi_hdc(), (HPEN)p);
}

static ui_brush_t ui_gdi_set_brush(ui_brush_t b) {
    ut_not_null(b);
    return (ui_brush_t)SelectBrush(ui_gdi_hdc(), b);
}

static uint32_t ui_gdi_color_rgb(ui_color_t c) {
    assert(ui_color_is_8bit(c));
    return (COLORREF)(c & 0xFFFFFFFF);
}

static COLORREF ui_gdi_color_ref(ui_color_t c) {
    return ui_gdi.color_rgb(c);
}

static ui_color_t ui_gdi_set_text_color(ui_color_t c) {
    return SetTextColor(ui_gdi_hdc(), ui_gdi_color_ref(c));
}

static ui_font_t ui_gdi_set_font(ui_font_t f) {
    ut_not_null(f);
    return (ui_font_t)SelectFont(ui_gdi_hdc(), (HFONT)f);
}

static void ui_gdi_begin(ui_image_t* image) {
    swear(ui_gdi_context.hdc == null, "no nested begin()/end()");
    if (image != null) {
        swear(image->bitmap != null);
        ui_gdi_context.hdc = CreateCompatibleDC((HDC)ui_app.canvas);
        ui_gdi_context.bitmap = SelectBitmap(ui_gdi_hdc(),
                                             (HBITMAP)image->bitmap);
    } else {
        ui_gdi_context.hdc = (HDC)ui_app.canvas;
        swear(ui_gdi_context.bitmap == null);
    }
    ui_gdi_context.font  = ui_gdi_set_font(ui_app.fm.regular.font);
    ui_gdi_context.pen   = ui_gdi_set_pen(ui_gdi_pen_hollow);
    ui_gdi_context.brush = ui_gdi_set_brush(ui_gdi_brush_hollow);
    ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), 0, 0,
        &ui_gdi_context.brush_origin));
    ui_color_t tc = ui_colors.get_color(ui_color_id_window_text);
    ui_gdi_context.text_color = ui_gdi_set_text_color(tc);
    ui_gdi_context.background_mode = SetBkMode(ui_gdi_hdc(), TRANSPARENT);
    ui_gdi_context.stretch_mode = SetStretchBltMode(ui_gdi_hdc(), HALFTONE);
}

static void ui_gdi_end(void) {
    ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(),
                   ui_gdi_context.brush_origin.x,
                   ui_gdi_context.brush_origin.y, null));
    ui_gdi_set_brush(ui_gdi_context.brush);
    ui_gdi_set_pen(ui_gdi_context.pen);
    ui_gdi_set_text_color(ui_gdi_context.text_color);
    SetBkMode(ui_gdi_hdc(), ui_gdi_context.background_mode);
    SetStretchBltMode(ui_gdi_hdc(), ui_gdi_context.stretch_mode);
    if (ui_gdi_context.hdc != (HDC)ui_app.canvas) {
        swear(ui_gdi_context.bitmap != null); // 1x1 bitmap
        SelectBitmap(ui_gdi_context.hdc, (HBITMAP)ui_gdi_context.bitmap);
        ut_fatal_win32err(DeleteDC(ui_gdi_context.hdc));
    }
    memset(&ui_gdi_context, 0x00, sizeof(ui_gdi_context));
}

static ui_pen_t ui_gdi_set_colored_pen(ui_color_t c) {
    ui_pen_t p = (ui_pen_t)SelectPen(ui_gdi_hdc(), GetStockPen(DC_PEN));
    SetDCPenColor(ui_gdi_hdc(), ui_gdi_color_ref(c));
    return p;
}

static ui_pen_t ui_gdi_create_pen(ui_color_t c, int32_t width) {
    assert(width >= 1);
    ui_pen_t pen = (ui_pen_t)CreatePen(PS_SOLID, width, ui_gdi_color_ref(c));
    ut_not_null(pen);
    return pen;
}

static void ui_gdi_delete_pen(ui_pen_t p) {
    ut_fatal_win32err(DeletePen(p));
}

static ui_brush_t ui_gdi_create_brush(ui_color_t c) {
    return (ui_brush_t)CreateSolidBrush(ui_gdi_color_ref(c));
}

static void ui_gdi_delete_brush(ui_brush_t b) {
    DeleteBrush((HBRUSH)b);
}

static ui_color_t ui_gdi_set_brush_color(ui_color_t c) {
    return SetDCBrushColor(ui_gdi_hdc(), ui_gdi_color_ref(c));
}

static void ui_gdi_set_clip(int32_t x, int32_t y, int32_t w, int32_t h) {
    if (ui_gdi_clip != null) { DeleteRgn(ui_gdi_clip); ui_gdi_clip = null; }
    if (w > 0 && h > 0) {
        ui_gdi_clip = (ui_region_t)CreateRectRgn(x, y, x + w, y + h);
        ut_not_null(ui_gdi_clip);
    }
    ut_fatal_if(SelectClipRgn(ui_gdi_hdc(), (HRGN)ui_gdi_clip) == ERROR);
}

static void ui_gdi_pixel(int32_t x, int32_t y, ui_color_t c) {
    ut_not_null(ui_app.canvas);
    ut_fatal_win32err(SetPixel(ui_gdi_hdc(), x, y, ui_gdi_color_ref(c)));
}

static void ui_gdi_rectangle(int32_t x, int32_t y, int32_t w, int32_t h) {
    ut_fatal_win32err(Rectangle(ui_gdi_hdc(), x, y, x + w, y + h));
}

static void ui_gdi_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1,
        ui_color_t c) {
    POINT pt;
    ut_fatal_win32err(MoveToEx(ui_gdi_hdc(), x0, y0, &pt));
    ui_pen_t p = ui_gdi_set_colored_pen(c);
    ut_fatal_win32err(LineTo(ui_gdi_hdc(), x1, y1));
    ui_gdi_set_pen(p);
    ut_fatal_win32err(MoveToEx(ui_gdi_hdc(), pt.x, pt.y, null));
}

static void ui_gdi_frame(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
    ui_brush_t b = ui_gdi_set_brush(ui_gdi_brush_hollow);
    ui_pen_t p = ui_gdi_set_colored_pen(c);
    ui_gdi_rectangle(x, y, w, h);
    ui_gdi_set_pen(p);
    ui_gdi_set_brush(b);
}

static void ui_gdi_rect(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t border, ui_color_t fill) {
    const bool tf = ui_color_is_transparent(fill);   // transparent fill
    const bool tb = ui_color_is_transparent(border); // transparent border
    ui_brush_t b = tf ? ui_gdi_brush_hollow : ui_gdi_brush_color;
    b = ui_gdi_set_brush(b);
    ui_color_t c = tf ? ui_colors.transparent : ui_gdi_set_brush_color(fill);
    ui_pen_t p = tb ? ui_gdi_set_pen(ui_gdi_pen_hollow) :
                      ui_gdi_set_colored_pen(border);
    ui_gdi_rectangle(x, y, w, h);
    if (!tf) { ui_gdi_set_brush_color(c); }
    ui_gdi_set_pen(p);
    ui_gdi_set_brush(b);
}

static void ui_gdi_fill(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t c) {
//  ut_traceln("%d,%d %dx%d 0x%08X", x, y, w, h, (uint32_t)c);
    ui_brush_t b = ui_gdi_set_brush(ui_gdi_brush_color);
    c = ui_gdi_set_brush_color(c);
    RECT rc = { x, y, x + w, y + h };
    HBRUSH brush = (HBRUSH)GetCurrentObject(ui_gdi_hdc(), OBJ_BRUSH);
    ut_fatal_win32err(FillRect(ui_gdi_hdc(), &rc, brush));
    ui_gdi_set_brush_color(c);
    ui_gdi_set_brush(b);
}

static void ui_gdi_poly(ui_point_t* points, int32_t count, ui_color_t c) {
    // make sure ui_point_t and POINT have the same memory layout:
    static_assert(sizeof(points->x) == sizeof(((POINT*)0)->x), "ui_point_t");
    static_assert(sizeof(points->y) == sizeof(((POINT*)0)->y), "ui_point_t");
    static_assert(sizeof(points[0]) == sizeof(*((POINT*)0)), "ui_point_t");
    assert(ui_gdi_hdc() != null && count > 1);
    ui_pen_t pen = ui_gdi_set_colored_pen(c);
    ut_fatal_win32err(Polyline(ui_gdi_hdc(), (POINT*)points, count));
    ui_gdi_set_pen(pen);
}

static void ui_gdi_circle(int32_t x, int32_t y, int32_t radius,
        ui_color_t border, ui_color_t fill) {
    swear(!ui_color_is_transparent(border) || ui_color_is_transparent(fill));
    // Win32 GDI even radius drawing looks ugly squarish and asymmetrical.
    swear(radius % 2 == 1, "radius: %d must be odd");
    if (ui_color_is_transparent(border)) {
        assert(!ui_color_is_transparent(fill));
        border = fill;
    }
    assert(!ui_color_is_transparent(border));
    const bool tf = ui_color_is_transparent(fill);   // transparent fill
    ui_brush_t brush = tf ? ui_gdi_set_brush(ui_gdi_brush_hollow) :
                        ui_gdi_set_brush(ui_gdi_brush_color);
    ui_color_t c = tf ? ui_colors.transparent : ui_gdi_set_brush_color(fill);
    ui_pen_t p = ui_gdi_set_colored_pen(border);
    HDC hdc = ui_gdi_context.hdc;
    int32_t l = x - radius;
    int32_t t = y - radius;
    int32_t r = x + radius + 1;
    int32_t b = y + radius + 1;
    Ellipse(hdc, l, t, r, b);
//  SetPixel(hdc, x, y, RGB(255, 255, 255));
    ui_gdi_set_pen(p);
    if (!tf) { ui_gdi_set_brush_color(c); }
    ui_gdi_set_brush(brush);
}

static void ui_gdi_fill_rounded(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t radius, ui_color_t fill) {
    int32_t r = x + w - 1; // right
    int32_t b = y + h - 1; // bottom
    ui_gdi_circle(x + radius, y + radius, radius, fill, fill);
    ui_gdi_circle(r - radius, y + radius, radius, fill, fill);
    ui_gdi_circle(x + radius, b - radius, radius, fill, fill);
    ui_gdi_circle(r - radius, b - radius, radius, fill, fill);
    // rectangles
    ui_gdi.fill(x + radius, y, w - radius * 2, h, fill);
    r = x + w - radius;
    ui_gdi.fill(x, y + radius, radius, h - radius * 2, fill);
    ui_gdi.fill(r, y + radius, radius, h - radius * 2, fill);
}

static void ui_gdi_rounded_border(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t radius, ui_color_t border) {
    {
        int32_t r = x + w - 1; // right
        int32_t b = y + h - 1; // bottom
        ui_gdi.set_clip(x, y, radius + 1, radius + 1);
        ui_gdi_circle(x + radius, y + radius, radius, border, ui_colors.transparent);
        ui_gdi.set_clip(r - radius, y, radius + 1, radius + 1);
        ui_gdi_circle(r - radius, y + radius, radius, border, ui_colors.transparent);
        ui_gdi.set_clip(x, b - radius, radius + 1, radius + 1);
        ui_gdi_circle(x + radius, b - radius, radius, border, ui_colors.transparent);
        ui_gdi.set_clip(r - radius, b - radius, radius + 1, radius + 1);
        ui_gdi_circle(r - radius, b - radius, radius, border, ui_colors.transparent);
        ui_gdi.set_clip(0, 0, 0, 0);
    }
    {
        int32_t r = x + w - 1; // right
        int32_t b = y + h - 1; // bottom
        ui_gdi.line(x + radius, y, r - radius + 1, y, border);
        ui_gdi.line(x + radius, b, r - radius + 1, b, border);
        ui_gdi.line(x - 1, y + radius, x - 1, b - radius + 1, border);
        ui_gdi.line(r + 1, y + radius, r + 1, b - radius + 1, border);
    }
}

static void ui_gdi_rounded(int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t radius, ui_color_t border, ui_color_t fill) {
    swear(!ui_color_is_transparent(border) || !ui_color_is_transparent(fill));
    if (!ui_color_is_transparent(fill)) {
        ui_gdi_fill_rounded(x, y, w, h, radius, fill);
    }
    if (!ui_color_is_transparent(border)) {
        ui_gdi_rounded_border(x, y, w, h, radius, border);
    }
}

static void ui_gdi_gradient(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_color_t rgba_from, ui_color_t rgba_to, bool vertical) {
    TRIVERTEX vertex[2] = {0};
    vertex[0].x = x;
    vertex[0].y = y;
    // TODO: colors:
    vertex[0].Red   = (COLOR16)(((rgba_from >>  0) & 0xFF) << 8);
    vertex[0].Green = (COLOR16)(((rgba_from >>  8) & 0xFF) << 8);
    vertex[0].Blue  = (COLOR16)(((rgba_from >> 16) & 0xFF) << 8);
    vertex[0].Alpha = (COLOR16)(((rgba_from >> 24) & 0xFF) << 8);
    vertex[1].x = x + w;
    vertex[1].y = y + h;
    vertex[1].Red   = (COLOR16)(((rgba_to >>  0) & 0xFF) << 8);
    vertex[1].Green = (COLOR16)(((rgba_to >>  8) & 0xFF) << 8);
    vertex[1].Blue  = (COLOR16)(((rgba_to >> 16) & 0xFF) << 8);
    vertex[1].Alpha = (COLOR16)(((rgba_to >> 24) & 0xFF) << 8);
    GRADIENT_RECT gRect = {0, 1};
    const uint32_t mode = vertical ?
        GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H;
    GradientFill(ui_gdi_hdc(), vertex, 2, &gRect, 1, mode);
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

static void ui_gdi_greyscale(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride, const uint8_t* pixels) {
    ut_fatal_if(stride != ((iw + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFO *bi = ui_gdi_greyscale_bitmap_info(); // global! not thread safe
        BITMAPINFOHEADER* bih = &bi->bmiHeader;
        bih->biWidth = iw;
        bih->biHeight = -ih; // top down image
        bih->biSizeImage = (DWORD)(w * abs(h));
        POINT pt = { 0 };
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), 0, 0, &pt));
        ut_fatal_if(StretchDIBits(ui_gdi_hdc(), sx, sy, sw, sh, x, y, w, h,
            pixels, bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFOHEADER ui_gdi_bgrx_init_bi(int32_t w, int32_t h, int32_t bpp) {
    assert(w > 0 && h >= 0); // h cannot be negative?
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

// bgr(iw) assumes strides are padded and rounded up to 4 bytes
// if this is not the case use ui_gdi.image_init() that will unpack
// and align scanlines prior to draw

static void ui_gdi_bgr(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride,
        const uint8_t* pixels) {
    ut_fatal_if(stride != ((iw * 3 + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFOHEADER bi = ui_gdi_bgrx_init_bi(iw, ih, 3);
        POINT pt = { 0 };
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), 0, 0, &pt));
        ut_fatal_if(StretchDIBits(ui_gdi_hdc(), sx, sy, sw, sh, x, y, w, h,
            pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), pt.x, pt.y, &pt));
    }
}

static void ui_gdi_bgrx(int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        int32_t x, int32_t y, int32_t w, int32_t h,
        int32_t iw, int32_t ih, int32_t stride,
        const uint8_t* pixels) {
    ut_fatal_if(stride != ((iw * 4 + 3) & ~0x3));
    assert(w > 0 && h != 0); // h can be negative
    if (w > 0 && h != 0) {
        BITMAPINFOHEADER bi = ui_gdi_bgrx_init_bi(iw, ih, 4);
        POINT pt = { 0 };
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), 0, 0, &pt));
        ut_fatal_if(StretchDIBits(ui_gdi_hdc(), sx, sy, sw, sh, x, y, w, h,
            pixels, (BITMAPINFO*)&bi, DIB_RGB_COLORS, SRCCOPY) == 0);
        ut_fatal_win32err(SetBrushOrgEx(ui_gdi_hdc(), pt.x, pt.y, &pt));
    }
}

static BITMAPINFO* ui_gdi_init_bitmap_info(int32_t w, int32_t h, int32_t bpp,
        BITMAPINFO* bi) {
    assert(w > 0 && h >= 0); // h cannot be negative?
    bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi->bmiHeader.biWidth = w;
    bi->bmiHeader.biHeight = -h;  // top down image
    bi->bmiHeader.biPlanes = 1;
    bi->bmiHeader.biBitCount = (uint16_t)(bpp * 8);
    bi->bmiHeader.biCompression = BI_RGB;
    bi->bmiHeader.biSizeImage = (DWORD)(w * abs(h) * bpp);
    return bi;
}

static void ui_gdi_create_dib_section(ui_image_t* image, int32_t w, int32_t h,
        int32_t bpp) {
    ut_fatal_if(image->bitmap != null, "image_dispose() not called?");
    // not using GetWindowDC(ui_app.window) will allow to initialize images
    // before window is created
    HDC c = CreateCompatibleDC(null); // GetWindowDC(ui_app.window);
    BITMAPINFO local = { {sizeof(BITMAPINFOHEADER)} };
    BITMAPINFO* bi = bpp == 1 ? ui_gdi_greyscale_bitmap_info() : &local;
    image->bitmap = (ui_bitmap_t)CreateDIBSection(c, ui_gdi_init_bitmap_info(w, h, bpp, bi),
                                               DIB_RGB_COLORS, &image->pixels, null, 0x0);
    ut_fatal_if(image->bitmap == null || image->pixels == null);
    ut_fatal_win32err(DeleteDC(c));
}

static void ui_gdi_image_init_rgbx(ui_image_t* image, int32_t w, int32_t h,
        int32_t bpp, const uint8_t* pixels) {
    bool swapped = bpp < 0;
    bpp = abs(bpp);
    ut_fatal_if(bpp != 4, "bpp: %d", bpp);
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
    ut_fatal_if(bpp < 0 || bpp == 2 || bpp > 4, "bpp=%d not {1, 3, 4}", bpp);
    ui_gdi_create_dib_section(image, w, h, bpp);
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

static void ui_gdi_alpha(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image, fp64_t alpha) {
    assert(image->bpp > 0);
    assert(0 <= alpha && alpha <= 1);
    ut_not_null(ui_gdi_hdc());
    HDC c = CreateCompatibleDC(ui_gdi_hdc());
    ut_not_null(c);
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
    ut_fatal_win32err(AlphaBlend(ui_gdi_hdc(), x, y, w, h,
        c, 0, 0, image->w, image->h, bf));
    SelectBitmap((HDC)c, zero1x1);
    ut_fatal_win32err(DeleteDC(c));
}

static void ui_gdi_image(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_image_t* image) {
    assert(image->bpp == 1 || image->bpp == 3 || image->bpp == 4);
    ut_not_null(ui_gdi_hdc());
    if (image->bpp == 1) { // StretchBlt() is bad for greyscale
        BITMAPINFO* bi = ui_gdi_greyscale_bitmap_info();
        ut_fatal_if(StretchDIBits(ui_gdi_hdc(), x, y, w, h, 0, 0, image->w, image->h,
            image->pixels, ui_gdi_init_bitmap_info(image->w, image->h, 1, bi),
            DIB_RGB_COLORS, SRCCOPY) == 0);
    } else {
        HDC c = CreateCompatibleDC(ui_gdi_hdc());
        ut_not_null(c);
        HBITMAP zero1x1 = SelectBitmap(c, image->bitmap);
        ut_fatal_win32err(StretchBlt(ui_gdi_hdc(), x, y, w, h,
            c, 0, 0, image->w, image->h, SRCCOPY));
        SelectBitmap(c, zero1x1);
        ut_fatal_win32err(DeleteDC(c));
    }
}

static void ui_gdi_icon(int32_t x, int32_t y, int32_t w, int32_t h,
        ui_icon_t icon) {
    DrawIconEx(ui_gdi_hdc(), x, y, (HICON)icon, w, h, 0, NULL, DI_NORMAL | DI_COMPAT);
}

static void ui_gdi_cleartype(bool on) {
    enum { spif = SPIF_UPDATEINIFILE | SPIF_SENDCHANGE };
    ut_fatal_win32err(SystemParametersInfoA(SPI_SETFONTSMOOTHING,
                                                   true, 0, spif));
    uintptr_t s = on ? FE_FONTSMOOTHINGCLEARTYPE : FE_FONTSMOOTHINGSTANDARD;
    ut_fatal_win32err(SystemParametersInfoA(SPI_SETFONTSMOOTHINGTYPE,
        0, (void*)s, spif));
}

static void ui_gdi_font_smoothing_contrast(int32_t c) {
    ut_fatal_if(!(c == -1 || 1000 <= c && c <= 2200), "contrast: %d", c);
    if (c == -1) { c = 1400; }
    ut_fatal_win32err(SystemParametersInfoA(SPI_SETFONTSMOOTHINGCONTRAST,
        0, (void*)(uintptr_t)c, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE));
}

ut_static_assertion(ui_gdi_font_quality_default == DEFAULT_QUALITY);
ut_static_assertion(ui_gdi_font_quality_draft == DRAFT_QUALITY);
ut_static_assertion(ui_gdi_font_quality_proof == PROOF_QUALITY);
ut_static_assertion(ui_gdi_font_quality_nonantialiased == NONANTIALIASED_QUALITY);
ut_static_assertion(ui_gdi_font_quality_antialiased == ANTIALIASED_QUALITY);
ut_static_assertion(ui_gdi_font_quality_cleartype == CLEARTYPE_QUALITY);
ut_static_assertion(ui_gdi_font_quality_cleartype_natural == CLEARTYPE_NATURAL_QUALITY);

static ui_font_t ui_gdi_create_font(const char* family, int32_t h, int32_t q) {
    assert(h > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(ui_app.fm.regular.font, sizeof(lf), &lf);
    ut_fatal_if(n != (int32_t)sizeof(lf));
    lf.lfHeight = -h;
    ut_str_printf(lf.lfFaceName, "%s", family);
    if (ui_gdi_font_quality_default <= q &&
        q <= ui_gdi_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)q;
    } else {
        ut_fatal_if(q != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}

static ui_font_t ui_gdi_font(ui_font_t f, int32_t h, int32_t q) {
    assert(f != null && h > 0);
    LOGFONTA lf = {0};
    int32_t n = GetObjectA(f, sizeof(lf), &lf);
    ut_fatal_if(n != (int32_t)sizeof(lf));
    lf.lfHeight = -h;
    if (ui_gdi_font_quality_default <= q &&
        q <= ui_gdi_font_quality_cleartype_natural) {
        lf.lfQuality = (uint8_t)q;
    } else {
        ut_fatal_if(q != -1, "use -1 for do not care quality");
    }
    return (ui_font_t)CreateFontIndirectA(&lf);
}

static void ui_gdi_delete_font(ui_font_t f) {
    ut_fatal_win32err(DeleteFont(f));
}

// guaranteed to return dc != null even if not painting

static HDC ui_gdi_get_dc(void) {
    ut_not_null(ui_app.window);
    HDC hdc = ui_gdi_hdc() != null ?
              ui_gdi_hdc() : GetDC((HWND)ui_app.window);
    ut_not_null(hdc);
    return hdc;
}

static void ui_gdi_release_dc(HDC hdc) {
    if (ui_gdi_hdc() == null) {
        ReleaseDC((HWND)ui_app.window, hdc);
    }
}

#define ui_gdi_with_hdc(code) do {           \
    HDC hdc = ui_gdi_get_dc();               \
    code                                     \
    ui_gdi_release_dc(hdc);                  \
} while (0)

#define ui_gdi_hdc_with_font(f, ...) do {    \
    ut_not_null(f);                          \
    HDC hdc = ui_gdi_get_dc();               \
    HFONT font_ = SelectFont(hdc, (HFONT)f); \
    { __VA_ARGS__ }                          \
    SelectFont(hdc, font_);                  \
    ui_gdi_release_dc(hdc);                  \
} while (0)

static void ui_gdi_dump_hdc_fm(HDC hdc) {
    // https://en.wikipedia.org/wiki/Quad_(typography)
    // https://learn.microsoft.com/en-us/windows/win32/gdi/string-widths-and-heights
    // https://stackoverflow.com/questions/27631736/meaning-of-top-ascent-baseline-descent-bottom-and-leading-in-androids-font
    // Amazingly same since Windows 3.1 1992
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-textmetrica
    // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-outlinetextmetrica
    TEXTMETRICA tm = {0};
    ut_fatal_win32err(GetTextMetricsA(hdc, &tm));
    char pitch[64] = { 0 };
    if (tm.tmPitchAndFamily & TMPF_FIXED_PITCH) { strcat(pitch, "FIXED_PITCH "); }
    if (tm.tmPitchAndFamily & TMPF_VECTOR)      { strcat(pitch, "VECTOR "); }
    if (tm.tmPitchAndFamily & TMPF_DEVICE)      { strcat(pitch, "DEVICE "); }
    if (tm.tmPitchAndFamily & TMPF_TRUETYPE)    { strcat(pitch, "TRUETYPE "); }
    ut_traceln("tm: .pitch_and_family: %s", pitch);
    ut_traceln(".height            : %2d   .ascent (baseline) : %2d  .descent: %2d",
            tm.tmHeight, tm.tmAscent, tm.tmDescent);
    ut_traceln(".internal_leading  : %2d   .external_leading  : %2d  .ave_char_width: %2d",
            tm.tmInternalLeading, tm.tmExternalLeading, tm.tmAveCharWidth);
    ut_traceln(".max_char_width    : %2d   .weight            : %2d .overhang: %2d",
            tm.tmMaxCharWidth, tm.tmWeight, tm.tmOverhang);
    ut_traceln(".digitized_aspect_x: %2d   .digitized_aspect_y: %2d",
            tm.tmDigitizedAspectX, tm.tmDigitizedAspectY);
    swear(tm.tmPitchAndFamily & TMPF_TRUETYPE);
    OUTLINETEXTMETRICA otm = { .otmSize = sizeof(OUTLINETEXTMETRICA) };
    uint32_t bytes = GetOutlineTextMetricsA(hdc, otm.otmSize, &otm);
    swear(bytes == sizeof(OUTLINETEXTMETRICA));
    // unsupported XHeight CapEmHeight
    // ignored:    MacDescent, MacLineGap, EMSquare, ItalicAngle
    //             CharSlopeRise, CharSlopeRun, ItalicAngle
    ut_traceln("otm: .Ascent       : %2d   .Descent        : %2d",
            otm.otmAscent, otm.otmDescent);
    ut_traceln(".otmLineGap        : %2u", otm.otmLineGap);
    ut_traceln(".FontBox.ltrb      :  %d,%d %2d,%2d",
            otm.otmrcFontBox.left, otm.otmrcFontBox.top,
            otm.otmrcFontBox.right, otm.otmrcFontBox.bottom);
    ut_traceln(".MinimumPPEM       : %2u    (minimum height in pixels)",
            otm.otmusMinimumPPEM);
    ut_traceln(".SubscriptOffset   : %d,%d  .SubscriptSize.x   : %dx%d",
            otm.otmptSubscriptOffset.x, otm.otmptSubscriptOffset.y,
            otm.otmptSubscriptSize.x, otm.otmptSubscriptSize.y);
    ut_traceln(".SuperscriptOffset : %d,%d  .SuperscriptSize.x : %dx%d",
            otm.otmptSuperscriptOffset.x, otm.otmptSuperscriptOffset.y,
            otm.otmptSuperscriptSize.x,   otm.otmptSuperscriptSize.y);
    ut_traceln(".UnderscoreSize    : %2d   .UnderscorePosition: %2d",
            otm.otmsUnderscoreSize, otm.otmsUnderscorePosition);
    ut_traceln(".StrikeoutSize     : %2u   .StrikeoutPosition : %2d ",
            otm.otmsStrikeoutSize,  otm.otmsStrikeoutPosition);
    int32_t h = otm.otmAscent + abs(tm.tmDescent); // without diacritical space above
    fp32_t pts = (h * 72.0f)  / GetDeviceCaps(hdc, LOGPIXELSY);
    ut_traceln("height: %.1fpt", pts);
}

static void ui_gdi_dump_fm(ui_font_t f) {
    ut_not_null(f);
    ui_gdi_hdc_with_font(f, { ui_gdi_dump_hdc_fm(hdc); });
}

static void ui_gdi_get_fm(HDC hdc, ui_fm_t* fm) {
    TEXTMETRICA tm = {0};
    ut_fatal_win32err(GetTextMetricsA(hdc, &tm));
    swear(tm.tmPitchAndFamily & TMPF_TRUETYPE);
    OUTLINETEXTMETRICA otm = { .otmSize = sizeof(OUTLINETEXTMETRICA) };
    uint32_t bytes = GetOutlineTextMetricsA(hdc, otm.otmSize, &otm);
    swear(bytes == sizeof(OUTLINETEXTMETRICA));
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
    fm->box = (ui_rect_t){
                otm.otmrcFontBox.left, otm.otmrcFontBox.top,
                otm.otmrcFontBox.right - otm.otmrcFontBox.left,
                otm.otmrcFontBox.bottom - otm.otmrcFontBox.top
    };
    // otm.Descent: The maximum distance characters in this font extend below
    // the base line. This is the typographic descent for the font.
    // Negative from the bottom (font.height)
    // tm.Descent: The descent (units below the base line) of characters.
    // Positive from the baseline down
    assert(tm.tmDescent >= 0 && otm.otmDescent <= 0 &&
           -otm.otmDescent <= tm.tmDescent,
           "tm.tmDescent: %d otm.otmDescent: %d", tm.tmDescent, otm.otmDescent);
    // "Mac" typography is ignored because it's usefulness is unclear.
    // Italic angle/slant/run is ignored because at the moment edit
    // view implementation does not support italics and thus does not
    // need it. Easy to add if necessary.
};

static void ui_gdi_update_fm(ui_fm_t* fm, ui_font_t f) {
    ut_not_null(f);
    SIZE em = {0, 0}; // "m"
    *fm = (ui_fm_t){ .font = f };
//  ui_gdi.dump_fm(f);
    ui_gdi_hdc_with_font(f, {
        ui_gdi_get_fm(hdc, fm);
        // ut_glyph_nbsp and "M" have the same result
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, "m", 1, &em));
        SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        SIZE e3 = {0}; // Three-Em Dash
        ut_fatal_win32err(GetTextExtentPoint32A(hdc,
            ut_glyph_three_em_dash, 1, &e3));
        fm->mono = em.cx == vl.cx && vl.cx == e3.cx;
//      ut_traceln("vl: %d %d", vl.cx, vl.cy);
//      ut_traceln("e3: %d %d", e3.cx, e3.cy);
//      ut_traceln("fm->mono: %d height: %d baseline: %d ascent: %d descent: %d",
//              fm->mono, fm->height, fm->baseline, fm->ascent, fm->descent);
    });
    assert(fm->baseline <= fm->height);
    fm->em = (ui_wh_t){ .w = fm->height, .h = fm->height };
//  ut_traceln("fm.em: %dx%d", fm->em.w, fm->em.h);
}

static int32_t ui_gdi_draw_utf16(ui_font_t font, const char* s, int32_t n,
        RECT* r, uint32_t format) { // ~70 microsecond Core i-7 3667U 2.0 GHz (2012)
    // if font == null, draws on HDC with selected font
if (0) {
    HDC hdc = ui_gdi_hdc();
    if (hdc != null) {
        SIZE em = {0, 0}; // "M"
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, "M", 1, &em));
        ut_traceln("em: %d %d", em.cx, em.cy);
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, ut_glyph_em_quad, 1, &em));
        ut_traceln("em: %d %d", em.cx, em.cy);
        SIZE vl = {0}; // "|" Vertical Line https://www.compart.com/en/unicode/U+007C
        SIZE e3 = {0}; // Three-Em Dash
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, "|", 1, &vl));
        ut_traceln("vl: %d %d", vl.cx, vl.cy);
        ut_fatal_win32err(GetTextExtentPoint32A(hdc, ut_glyph_three_em_dash, 1, &e3));
        ut_traceln("e3: %d %d", e3.cx, e3.cy);
    }
}
    int32_t count = ut_str.utf16_chars(s, -1);
    assert(0 < count && count < 4096, "be reasonable count: %d?", count);
    uint16_t ws[4096];
    swear(count <= ut_count_of(ws), "find another way to draw!");
    ut_str.utf8to16(ws, count, s, -1);
    int32_t h = 0; // return value is the height of the text
    if (font != null) {
        ui_gdi_hdc_with_font(font, { h = DrawTextW(hdc, ws, n, r, format); });
    } else { // with already selected font
        ui_gdi_with_hdc({ h = DrawTextW(hdc, ws, n, r, format); });
    }
    return h;
}

typedef struct { // draw text parameters
    const ui_fm_t* fm;
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
} ui_gdi_dtp_t;

static void ui_gdi_text_draw(ui_gdi_dtp_t* p) {
    ut_not_null(p);
    char text[4096]; // expected to be enough for single text draw
    text[0] = 0;
    ut_str.format_va(text, ut_count_of(text), p->format, p->va);
    text[ut_count_of(text) - 1] = 0;
    int32_t k = (int32_t)ut_str.len(text);
    if (k > 0) {
        swear(k > 0 && k < ut_count_of(text), "k=%d n=%d fmt=%s", k, p->format);
        // rectangle is always calculated - it makes draw text
        // much slower but UI layer is mostly uses bitmap caching:
        if ((p->flags & DT_CALCRECT) == 0) {
            // no actual drawing just calculate rectangle
            bool b = ui_gdi_draw_utf16(p->fm->font, text, -1, &p->rc, p->flags | DT_CALCRECT);
            assert(b, "text_utf16(%s) failed", text); (void)b;
        }
        bool b = ui_gdi_draw_utf16(p->fm->font, text, -1, &p->rc, p->flags);
        assert(b, "text_utf16(%s) failed", text); (void)b;
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

static ui_wh_t ui_gdi_text_with_flags(const ui_gdi_ta_t* ta,
        int32_t x, int32_t y, int32_t w,
        const char* format, va_list va, uint32_t flags) {
    const int32_t right = w == 0 ? 0 : x + w;
    ui_gdi_dtp_t p = {
        .fm = ta->fm,
        .format = format,
        .va = va,
        .rc = {.left = x, .top = y, .right = right, .bottom = 0 },
        .flags = flags
    };
    ui_color_t c = ta->color;
    if (!ta->measure) {
        if (ui_color_is_undefined(c)) {
            swear(ta->color_id > 0);
            c = ui_colors.get_color(ta->color_id);
        } else {
            swear(ta->color_id == 0);
        }
        c = ui_gdi_set_text_color(c);
    }
    ui_gdi_text_draw(&p);
    if (!ta->measure) { ui_gdi_set_text_color(c); } // restore color
    return (ui_wh_t){ p.rc.right - p.rc.left, p.rc.bottom - p.rc.top };
}

static ui_wh_t ui_gdi_text_va(const ui_gdi_ta_t* ta,
        int32_t x, int32_t y,  const char* format, va_list va) {
    const uint32_t flags = sl_draw | (ta->measure ? sl_measure : 0);
    return ui_gdi_text_with_flags(ta, x, y, 0, format, va, flags);
}

static ui_wh_t ui_gdi_text(const ui_gdi_ta_t* ta,
        int32_t x, int32_t y, const char* format, ...) {
    const uint32_t flags = sl_draw | (ta->measure ? sl_measure : 0);
    va_list va;
    va_start(va, format);
    ui_wh_t wh = ui_gdi_text_with_flags(ta, x, y, 0, format, va, flags);
    va_end(va);
    return wh;
}

static ui_wh_t ui_gdi_multiline_va(const ui_gdi_ta_t* ta,
        int32_t x, int32_t y, int32_t w, const char* format, va_list va) {
    const uint32_t flags = ta->measure ?
                            (w <= 0 ? ml_measure : ml_measure_break) :
                            (w <= 0 ? ml_draw    : ml_draw_break);
    return ui_gdi_text_with_flags(ta, x, y, w, format, va, flags);
}

static ui_wh_t ui_gdi_multiline(const ui_gdi_ta_t* ta,
        int32_t x, int32_t y, int32_t w, const char* format, ...) {
    va_list va;
    va_start(va, format);
    ui_wh_t wh = ui_gdi_multiline_va(ta, x, y, w, format, va);
    va_end(va);
    return wh;
}

static ui_wh_t ui_gdi_glyphs_placement(const ui_gdi_ta_t* ta,
        const char* utf8, int32_t bytes, int32_t x[], int32_t glyphs) {
    swear(bytes >= 0 && glyphs >= 0 && glyphs <= bytes);
    assert(false, "Does not work for Tamil simplest utf8: \xe0\xae\x9a utf16: 0x0B9A");
    x[0] = 0;
    ui_wh_t wh = { .w = 0, .h = 0 };
    if (bytes > 0) {
        const int32_t chars = ut_str.utf16_chars(utf8, bytes);
        uint16_t* utf16 = ut_stackalloc((chars + 1) * sizeof(uint16_t));
        uint16_t* output = ut_stackalloc((chars + 1) * sizeof(uint16_t));
        const errno_t r = ut_str.utf8to16(utf16, chars, utf8, bytes);
        swear(r == 0);
// TODO: remove
#if 1
        char str[16 * 1024] = {0};
        char hex[16 * 1024] = {0};
        for (int i = 0; i < chars; i++) {
            ut_str_printf(hex, "%04X ", utf16[i]);
            strcat(str, hex);
        }
ut_traceln("%.*s %s %p bytes:%d glyphs:%d font:%p hdc:%p", bytes, utf8, str, utf8, bytes, glyphs, ta->fm->font, ui_gdi_context.hdc);
#endif
        GCP_RESULTSW gcp = {
            .lStructSize = sizeof(GCP_RESULTSW),
            .lpOutString = output,
            .nGlyphs = glyphs
        };
        gcp.lpDx = (int*)ut_stackalloc((chars + 1) * sizeof(int));
        DWORD n = 0;
        const int mx = INT32_MAX; // max extent
        const DWORD f = GCP_MAXEXTENT; // |GCP_GLYPHSHAPE|GCP_DIACRITIC|GCP_LIGATE
        if (ta->fm->font != null) {
            ui_gdi_hdc_with_font(ta->fm->font, {
                n = GetCharacterPlacementW(hdc, utf16, chars, mx, &gcp, f);
            });
        } else { // with already selected font
            ui_gdi_with_hdc({
                n = GetCharacterPlacementW(hdc, utf16, chars, mx, &gcp, f);
            });
        }
        wh = (ui_wh_t){ .w = LOWORD(n), .h = HIWORD(n) };
        if (n != 0) {
            // IS_HIGH_SURROGATE(wch)
            // IS_LOW_SURROGATE(wch)
            // IS_SURROGATE_PAIR(hs, ls)
            int32_t i = 0;
            int32_t k = 1;
            while (i < chars) {
                x[k] = x[k - 1] + gcp.lpDx[i];
//              ut_traceln("%d", x[i]);
                k++;
                if (i < chars - 1 && ut_str.utf16_is_high_surrogate(utf16[i]) &&
                                     ut_str.utf16_is_low_surrogate(utf16[i + 1])) {
                    i += 2;
                } else {
                    i++;
                }
            }
            assert(k == glyphs + 1);
        } else {
//          assert(false, "GetCharacterPlacementW() failed");
            ut_traceln("GetCharacterPlacementW() failed");
        }
    }
    return wh;
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
        ut_fatal_if(true, "curl.exe --silent --fail --create-dirs "
            "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h "
            "--output ext/stb_image.h");
        return null;
    #endif
}

static void ui_gdi_image_dispose(ui_image_t* image) {
    ut_fatal_win32err(DeleteBitmap(image->bitmap));
    memset(image, 0, sizeof(ui_image_t));
}

ui_gdi_if ui_gdi = {
    .ta = {
        .regular = {
            .color_id = ui_color_id_window_text,
            .color    = ui_color_undefined,
            .fm       = &ui_app.fm.regular,
            .measure  = false
        },
        .mono = {
            .color_id = ui_color_id_window_text,
            .color    = ui_color_undefined,
            .fm       = &ui_app.fm.mono,
            .measure  = false
        },
        .H1 = {
            .color_id = ui_color_id_window_text,
            .color    = ui_color_undefined,
            .fm       = &ui_app.fm.H1,
            .measure  = false
        },
        .H2 = {
            .color_id = ui_color_id_window_text,
            .color    = ui_color_undefined,
            .fm       = &ui_app.fm.H2,
            .measure  = false
        },
        .H3 = {
            .color_id = ui_color_id_window_text,
            .color    = ui_color_undefined,
            .fm       = &ui_app.fm.H3,
            .measure  = false
        }
    },
    .init                     = ui_gdi_init,
    .begin                    = ui_gdi_begin,
    .end                      = ui_gdi_end,
    .color_rgb                = ui_gdi_color_rgb,
    .image_init               = ui_gdi_image_init,
    .image_init_rgbx          = ui_gdi_image_init_rgbx,
    .image_dispose            = ui_gdi_image_dispose,
    .alpha                    = ui_gdi_alpha,
    .image                    = ui_gdi_image,
    .icon                     = ui_gdi_icon,
    .set_clip                 = ui_gdi_set_clip,
    .pixel                    = ui_gdi_pixel,
    .line                     = ui_gdi_line,
    .frame                    = ui_gdi_frame,
    .rect                     = ui_gdi_rect,
    .fill                     = ui_gdi_fill,
    .poly                     = ui_gdi_poly,
    .circle                   = ui_gdi_circle,
    .rounded                  = ui_gdi_rounded,
    .gradient                 = ui_gdi_gradient,
    .greyscale                = ui_gdi_greyscale,
    .bgr                      = ui_gdi_bgr,
    .bgrx                     = ui_gdi_bgrx,
    .cleartype                = ui_gdi_cleartype,
    .font_smoothing_contrast  = ui_gdi_font_smoothing_contrast,
    .create_font              = ui_gdi_create_font,
    .font                     = ui_gdi_font,
    .delete_font              = ui_gdi_delete_font,
    .dump_fm                  = ui_gdi_dump_fm,
    .update_fm                = ui_gdi_update_fm,
    .text_va                  = ui_gdi_text_va,
    .text                     = ui_gdi_text,
    .multiline_va             = ui_gdi_multiline_va,
    .multiline                = ui_gdi_multiline,
    .glyphs_placement         = ui_gdi_glyphs_placement,
    .fini                     = ui_gdi_fini
};

#pragma pop_macro("ui_gdi_hdc_with_font")
#pragma pop_macro("ui_gdi_with_hdc")
