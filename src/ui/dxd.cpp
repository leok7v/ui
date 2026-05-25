// Direct2D + DirectWrite drawing backend for ui_gdi. See inc/ui/dxd.h.
// The rt/ui headers are C; compiling them as C++ trips benign warnings
// (deleted special members on const-field structs, nameless unions) under
// /Wall -- silence those around the includes only.
#pragma warning(push)
#pragma warning(disable: 4201 4459 4623 4625 4626 5026 5027 5039)
#include "rt/rt.h"
#include "ui/ui.h"
#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <stdlib.h>
#pragma warning(pop)

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// "no width/height constraint" for DirectWrite text layout (not a DWrite
// macro -- a large finite float so layout math stays well-defined).
#define DWRITE_MEASURE_MAX_WIDTH ((FLOAT)1.0e6f)

static ID2D1Factory *  g_d2d_factory    = null;
static IDWriteFactory * g_dwrite_factory = null;
static ID2D1DCRenderTarget * g_target = null;

struct dxd_context_s {
    ID2D1DCRenderTarget * target;
    ID2D1SolidColorBrush * brush;
    ui_color_t brush_color;
    bool brush_valid;
};

static D2D1_COLOR_F dxd_color_f(ui_color_t c) {
    // ui_color_rgb leaves the alpha byte 0 to denote an opaque color.
    uint8_t a = ui_color_a(c);
    if (a == 0) { a = 0xFF; }
    D2D1_COLOR_F color = {
        (FLOAT)ui_color_r(c) / 255.0f,
        (FLOAT)ui_color_g(c) / 255.0f,
        (FLOAT)ui_color_b(c) / 255.0f,
        (FLOAT)a / 255.0f
    };
    return color;
}

static ID2D1SolidColorBrush * dxd_get_brush(dxd_context_t ctx, ui_color_t c) {
    ID2D1SolidColorBrush * b = null;
    if (ctx != null && ctx->target != null) {
        if (ctx->brush == null || !ctx->brush_valid || ctx->brush_color != c) {
            if (ctx->brush != null) { ctx->brush->Release(); ctx->brush = null; }
            HRESULT hr = ctx->target->CreateSolidColorBrush(dxd_color_f(c),
                                                            &ctx->brush);
            ctx->brush_valid = SUCCEEDED(hr);
            if (ctx->brush_valid) { ctx->brush_color = c; }
        }
        b = ctx->brush;
    }
    return b;
}

bool dxd_init(void) {
    bool ok = true;
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                   &g_d2d_factory);
    if (FAILED(hr)) { ok = false; }
    if (ok) {
        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                __uuidof(IDWriteFactory),
                reinterpret_cast<IUnknown **>(&g_dwrite_factory));
        if (FAILED(hr)) { ok = false; }
    }
    return ok;
}

void dxd_fini(void) {
    if (g_target != null) { g_target->Release(); g_target = null; }
    if (g_dwrite_factory != null) { g_dwrite_factory->Release(); g_dwrite_factory = null; }
    if (g_d2d_factory != null) { g_d2d_factory->Release(); g_d2d_factory = null; }
}

dxd_context_t dxd_begin(void * hdc, const ui_rect_t * rc) {
    dxd_context_t ctx = (dxd_context_t)malloc(sizeof(struct dxd_context_s));
    if (ctx != null) {
        ctx->target = null;
        ctx->brush = null;
        ctx->brush_color = 0;
        ctx->brush_valid = false;
        if (g_target == null) {
            D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                                  D2D1_ALPHA_MODE_PREMULTIPLIED),
                0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);
            if (FAILED(g_d2d_factory->CreateDCRenderTarget(&props, &g_target))) {
                g_target = null;
            }
        }
        RECT rect = { rc->x, rc->y, rc->x + rc->w, rc->y + rc->h };
        if (g_target != null && SUCCEEDED(g_target->BindDC((HDC)hdc, &rect))) {
            ctx->target = g_target;
            g_target->BeginDraw();
            g_target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        } else {
            free(ctx);
            ctx = null;
        }
    }
    return ctx;
}

void dxd_end(dxd_context_t ctx) {
    if (ctx != null) {
        if (ctx->brush != null) { ctx->brush->Release(); }
        if (ctx->target != null) {
            ctx->target->EndDraw();
        }
        free(ctx);
    }
}

void dxd_set_clip(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (ctx != null && ctx->target != null) {
        if (w > 0 && h > 0) {
            ctx->target->PushAxisAlignedClip(
                D2D1::RectF((FLOAT)x, (FLOAT)y, (FLOAT)(x + w), (FLOAT)(y + h)),
                D2D1_ANTIALIAS_MODE_ALIASED);
        } else {
            ctx->target->PopAxisAlignedClip();
        }
    }
}

void dxd_pixel(dxd_context_t ctx, int32_t x, int32_t y, ui_color_t color) {
    if (ctx != null && ctx->target != null) {
        ID2D1SolidColorBrush * b = dxd_get_brush(ctx, color);
        if (b != null) {
            ctx->target->FillRectangle(
                D2D1::RectF((FLOAT)x, (FLOAT)y, (FLOAT)(x + 1), (FLOAT)(y + 1)), b);
        }
    }
}

void dxd_line(dxd_context_t ctx, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
              ui_color_t color) {
    if (ctx != null && ctx->target != null) {
        ID2D1SolidColorBrush * b = dxd_get_brush(ctx, color);
        if (b != null) {
            ctx->target->DrawLine(D2D1::Point2F((FLOAT)x0, (FLOAT)y0),
                                  D2D1::Point2F((FLOAT)x1, (FLOAT)y1), b, 1.0f);
        }
    }
}

void dxd_frame(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h,
               ui_color_t color) {
    if (ctx != null && ctx->target != null) {
        ID2D1SolidColorBrush * b = dxd_get_brush(ctx, color);
        if (b != null) {
            ctx->target->DrawRectangle(
                D2D1::RectF((FLOAT)x, (FLOAT)y, (FLOAT)(x + w), (FLOAT)(y + h)),
                b, 1.0f);
        }
    }
}

void dxd_rect(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h,
              ui_color_t border, ui_color_t fill) {
    if (ctx != null && ctx->target != null) {
        D2D1_RECT_F r = D2D1::RectF((FLOAT)x, (FLOAT)y,
                                    (FLOAT)(x + w), (FLOAT)(y + h));
        if (!ui_color_is_transparent(fill)) {
            ID2D1SolidColorBrush * fb = dxd_get_brush(ctx, fill);
            if (fb != null) { ctx->target->FillRectangle(r, fb); }
        }
        if (!ui_color_is_transparent(border)) {
            ID2D1SolidColorBrush * bb = dxd_get_brush(ctx, border);
            if (bb != null) { ctx->target->DrawRectangle(r, bb, 1.0f); }
        }
    }
}

void dxd_fill(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h,
              ui_color_t color) {
    if (ctx != null && ctx->target != null) {
        ID2D1SolidColorBrush * b = dxd_get_brush(ctx, color);
        if (b != null) {
            ctx->target->FillRectangle(
                D2D1::RectF((FLOAT)x, (FLOAT)y, (FLOAT)(x + w), (FLOAT)(y + h)), b);
        }
    }
}

void dxd_poly(dxd_context_t ctx, ui_point_t * points, int32_t count,
              ui_color_t color) {
    if (ctx != null && ctx->target != null && count > 1) {
        ID2D1SolidColorBrush * b = dxd_get_brush(ctx, color);
        if (b != null) {
            ID2D1PathGeometry * geo = null;
            HRESULT hr = g_d2d_factory->CreatePathGeometry(&geo);
            if (SUCCEEDED(hr)) {
                ID2D1GeometrySink * sink = null;
                hr = geo->Open(&sink);
                if (SUCCEEDED(hr)) {
                    sink->BeginFigure(
                        D2D1::Point2F((FLOAT)points[0].x, (FLOAT)points[0].y),
                        D2D1_FIGURE_BEGIN_HOLLOW);
                    for (int32_t i = 1; i < count; i++) {
                        sink->AddLine(D2D1::Point2F((FLOAT)points[i].x,
                                                    (FLOAT)points[i].y));
                    }
                    sink->EndFigure(D2D1_FIGURE_END_OPEN);
                    sink->Close();
                    ctx->target->DrawGeometry(geo, b, 1.0f);
                    sink->Release();
                }
                geo->Release();
            }
        }
    }
}

void dxd_circle(dxd_context_t ctx, int32_t x, int32_t y, int32_t radius,
                ui_color_t border, ui_color_t fill) {
    if (ctx != null && ctx->target != null) {
        D2D1_ELLIPSE ell = D2D1::Ellipse(D2D1::Point2F((FLOAT)x, (FLOAT)y),
                                         (FLOAT)radius, (FLOAT)radius);
        if (!ui_color_is_transparent(fill)) {
            ID2D1SolidColorBrush * fb = dxd_get_brush(ctx, fill);
            if (fb != null) { ctx->target->FillEllipse(&ell, fb); }
        }
        if (!ui_color_is_transparent(border)) {
            ID2D1SolidColorBrush * bb = dxd_get_brush(ctx, border);
            if (bb != null) { ctx->target->DrawEllipse(&ell, bb, 1.0f); }
        }
    }
}

void dxd_rounded(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h,
                 int32_t radius, ui_color_t border, ui_color_t fill) {
    if (ctx != null && ctx->target != null) {
        D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(
            D2D1::RectF((FLOAT)x, (FLOAT)y, (FLOAT)(x + w), (FLOAT)(y + h)),
            (FLOAT)radius, (FLOAT)radius);
        if (!ui_color_is_transparent(fill)) {
            ID2D1SolidColorBrush * fb = dxd_get_brush(ctx, fill);
            if (fb != null) { ctx->target->FillRoundedRectangle(&rr, fb); }
        }
        if (!ui_color_is_transparent(border)) {
            ID2D1SolidColorBrush * bb = dxd_get_brush(ctx, border);
            if (bb != null) { ctx->target->DrawRoundedRectangle(&rr, bb, 1.0f); }
        }
    }
}

void dxd_gradient(dxd_context_t ctx, int32_t x, int32_t y, int32_t w, int32_t h,
                  ui_color_t c0, ui_color_t c1, bool vertical) {
    if (ctx != null && ctx->target != null) {
        ID2D1GradientStopCollection * stops = null;
        D2D1_GRADIENT_STOP gs[2] = {};
        gs[0].position = 0.0f; gs[0].color = dxd_color_f(c0);
        gs[1].position = 1.0f; gs[1].color = dxd_color_f(c1);
        HRESULT hr = ctx->target->CreateGradientStopCollection(
            gs, 2, D2D1_GAMMA_2_2, D2D1_EXTEND_MODE_CLAMP, &stops);
        if (SUCCEEDED(hr)) {
            ID2D1LinearGradientBrush * brush = null;
            D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES props =
                D2D1::LinearGradientBrushProperties(
                    D2D1::Point2F((FLOAT)x, (FLOAT)y),
                    vertical ? D2D1::Point2F((FLOAT)x, (FLOAT)(y + h))
                             : D2D1::Point2F((FLOAT)(x + w), (FLOAT)y));
            hr = ctx->target->CreateLinearGradientBrush(props, stops, &brush);
            if (SUCCEEDED(hr)) {
                ctx->target->FillRectangle(
                    D2D1::RectF((FLOAT)x, (FLOAT)y,
                                (FLOAT)(x + w), (FLOAT)(y + h)), brush);
                brush->Release();
            }
            stops->Release();
        }
    }
}

void dxd_image(dxd_context_t ctx, int32_t dx, int32_t dy, int32_t dw, int32_t dh,
               int32_t sx, int32_t sy, int32_t sw, int32_t sh,
               int32_t w, int32_t h, int32_t stride, int32_t bpp,
               const uint8_t * pixels, fp64_t opacity, bool premultiplied) {
    if (ctx == null || ctx->target == null || w <= 0 || h <= 0 ||
        pixels == null) {
        return;
    }
    const bool keep_alpha = premultiplied && bpp == 4;
    uint8_t * bgra = (uint8_t *)malloc((size_t)w * (size_t)h * 4);
    if (bgra == null) { return; }
    for (int32_t yy = 0; yy < h; yy++) {
        const uint8_t * src = pixels + (size_t)yy * (size_t)stride;
        uint8_t * dst = bgra + (size_t)yy * (size_t)w * 4;
        for (int32_t xx = 0; xx < w; xx++) {
            if (bpp == 1) {
                uint8_t g = src[xx];
                dst[0] = g; dst[1] = g; dst[2] = g; dst[3] = 0xFF;
            } else if (bpp == 3) {
                dst[0] = src[xx * 3 + 0];
                dst[1] = src[xx * 3 + 1];
                dst[2] = src[xx * 3 + 2];
                dst[3] = 0xFF;
            } else { // bpp == 4
                dst[0] = src[xx * 4 + 0];
                dst[1] = src[xx * 4 + 1];
                dst[2] = src[xx * 4 + 2];
                dst[3] = keep_alpha ? src[xx * 4 + 3] : 0xFF;
            }
            dst += 4;
        }
    }
    ID2D1Bitmap * bmp = null;
    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
            keep_alpha ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE));
    HRESULT hr = ctx->target->CreateBitmap(D2D1::SizeU((UINT32)w, (UINT32)h),
        bgra, (UINT32)(w * 4), &props, &bmp);
    if (SUCCEEDED(hr) && bmp != null) {
        D2D1_RECT_F d = D2D1::RectF((FLOAT)dx, (FLOAT)dy,
                                    (FLOAT)(dx + dw), (FLOAT)(dy + dh));
        D2D1_RECT_F s = D2D1::RectF((FLOAT)sx, (FLOAT)sy,
                                    (FLOAT)(sx + sw), (FLOAT)(sy + sh));
        ctx->target->DrawBitmap(bmp, d, (FLOAT)opacity,
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, s);
        bmp->Release();
    }
    free(bgra);
}

// Build a DirectWrite text format from a GDI HFONT's LOGFONT (caller releases).
static IDWriteTextFormat * dxd_format_for(ui_font_t font) {
    IDWriteTextFormat * fmt = null;
    LOGFONTW lf = {0};
    if (font != null && GetObjectW((HFONT)font, sizeof(lf), &lf) != 0) {
        FLOAT size = (FLOAT)(lf.lfHeight < 0 ? -lf.lfHeight : lf.lfHeight);
        if (size <= 0) { size = 12.0f; }
        DWRITE_FONT_WEIGHT weight = (DWRITE_FONT_WEIGHT)(lf.lfWeight > 0 ?
            lf.lfWeight : DWRITE_FONT_WEIGHT_NORMAL);
        g_dwrite_factory->CreateTextFormat(lf.lfFaceName, null, weight,
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size,
            L"en-us", &fmt);
    }
    return fmt;
}

ui_wh_t dxd_text(dxd_context_t ctx, ui_font_t font, int32_t x, int32_t y,
                 int32_t w, ui_color_t color, const char * utf8, int32_t bytes,
                 bool measure_only, bool multiline) {
    ui_wh_t size = {0, 0};
    IDWriteTextFormat * fmt = dxd_format_for(font);
    if (fmt != null && bytes > 0) {
        int32_t chars = MultiByteToWideChar(CP_UTF8, 0, utf8, bytes, null, 0);
        if (chars > 0) {
            wchar_t * wtext = (wchar_t *)malloc((size_t)chars * sizeof(wchar_t));
            if (wtext != null) {
                MultiByteToWideChar(CP_UTF8, 0, utf8, bytes, wtext, chars);
                IDWriteTextLayout * layout = null;
                FLOAT max_w = (multiline && w > 0) ? (FLOAT)w :
                    DWRITE_MEASURE_MAX_WIDTH;
                HRESULT hr = g_dwrite_factory->CreateTextLayout(wtext,
                    (UINT32)chars, fmt, max_w, DWRITE_MEASURE_MAX_WIDTH, &layout);
                if (SUCCEEDED(hr)) {
                    if (!multiline) {
                        layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    }
                    DWRITE_TEXT_METRICS m = {0};
                    layout->GetMetrics(&m);
                    size.w = (int32_t)(m.widthIncludingTrailingWhitespace + 0.5f);
                    size.h = (int32_t)(m.height + 0.5f);
                    if (!measure_only && ctx != null && ctx->target != null) {
                        ID2D1SolidColorBrush * b = dxd_get_brush(ctx, color);
                        if (b != null) {
                            ctx->target->DrawTextLayout(
                                D2D1::Point2F((FLOAT)x, (FLOAT)y), layout, b);
                        }
                    }
                    layout->Release();
                }
                free(wtext);
            }
        }
        fmt->Release();
    }
    return size;
}

ui_wh_t dxd_glyphs_placement(ui_font_t font, const char * utf8, int32_t bytes,
                             int32_t x_out[], int32_t glyphs) {
    ui_wh_t size = {0, 0};
    IDWriteTextFormat * fmt = dxd_format_for(font);
    if (fmt != null && bytes > 0) {
        int32_t chars = MultiByteToWideChar(CP_UTF8, 0, utf8, bytes, null, 0);
        if (chars > 0) {
            wchar_t * wtext = (wchar_t *)malloc((size_t)chars * sizeof(wchar_t));
            if (wtext != null) {
                MultiByteToWideChar(CP_UTF8, 0, utf8, bytes, wtext, chars);
                IDWriteTextLayout * layout = null;
                HRESULT hr = g_dwrite_factory->CreateTextLayout(wtext,
                    (UINT32)chars, fmt, DWRITE_MEASURE_MAX_WIDTH,
                    DWRITE_MEASURE_MAX_WIDTH, &layout);
                if (SUCCEEDED(hr)) {
                    layout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
                    UINT32 n = 0;
                    layout->GetClusterMetrics(null, 0, &n);
                    if (n > 0) {
                        DWRITE_CLUSTER_METRICS * cm = (DWRITE_CLUSTER_METRICS *)
                            malloc((size_t)n * sizeof(DWRITE_CLUSTER_METRICS));
                        if (cm != null) {
                            hr = layout->GetClusterMetrics(cm, n, &n);
                            if (SUCCEEDED(hr)) {
                                FLOAT cx = 0.0f;
                                int32_t out = 0;
                                x_out[0] = 0;
                                for (UINT32 i = 0; i < n; i++) {
                                    cx += cm[i].width;
                                    if (out < glyphs) {
                                        out++;
                                        x_out[out] = (int32_t)(cx + 0.5f);
                                    }
                                }
                            }
                            free(cm);
                        }
                    }
                    DWRITE_TEXT_METRICS m = {0};
                    layout->GetMetrics(&m);
                    size.w = (int32_t)(m.widthIncludingTrailingWhitespace + 0.5f);
                    size.h = (int32_t)(m.height + 0.5f);
                    layout->Release();
                }
                free(wtext);
            }
        }
        fmt->Release();
    }
    return size;
}
