#include "rt/rt.h"
#include "ui/ui.h"

rt_begin_c

// "iv" stands for "image view"

// To enable zoom/pan make view focusable:
// iv.focusable = true;

// Field .image may have .pixels pointer and .bitmap == null.
// If this is the case the direct pixels transfer to the
// device is used. RGBA bitmaps must be allocated on the
// device otherwise ui_gdi.rgbx() call is used and alpha
// is ignored.

typedef struct ui_iv_s ui_iv_t;

typedef struct ui_iv_s {
    union {
        ui_view_t view;
        struct ui_view_s;
    };
    ui_image_t image; // view does NOT own or dispose image->bitmap
    fp64_t     alpha; // for rgba images
    // actual scale() is: z = 2 ^ (zn - 1) / 2 ^ (zd - 1)
    int32_t zoom; // 0..8
    // 0=16:1 1=8:1 2=4:1 3=2:1 4=1:1 5=1:2 6=1:4 7=1:8 8=1:16
    int32_t zn; // zoom nominator (1, 2, 3, ...)
    int32_t zd; // zoom denominator (1, 2, 3, ...)
    fp64_t  sx; // shift x [0..1.0] in view coordinates
    fp64_t  sy; // shift y [0..1.0]
    struct { // only visible when focused
        ui_view_t   bar; // ui_view(span) {zoom in, zoom 1:1, zoom out, help}
        ui_button_t copy; // copy image to clipboard
        ui_button_t zoom_in;
        ui_button_t zoom_1t1; // 1:1
        ui_button_t zoom_out;
        ui_button_t fit;
        ui_button_t fill;
        ui_button_t help;
        ui_label_t  ratio;
    } tool;
    ui_point_t drag_start;
    fp64_t when; // to hide toolbar
    bool fit;    // best fit into view
    bool fill;   // fill entire view
    // fit and fill cannot be true at the same time
    // when fit: false and fill: false the zoom ratio is in effect
} ui_iv_t;

typedef struct ui_iv_if {
    void      (*init)(ui_iv_t* iv);
    void      (*init_with)(ui_iv_t* iv, const uint8_t* pixels,
                           int32_t width, int32_t height,
                           int32_t bpp, int32_t stride);
    // ration can only be: 16:1 8:1 4:1 2:1 1:1 1:2 1:4 1:8 1:16
    // but ignored if .fit or .fill is true
    void      (*ratio)(ui_iv_t* iv, int32_t nominator, int32_t denominator);
    fp64_t    (*scale)(ui_iv_t* iv); // 2 ^ (zn - 1) / 2 ^ (zd - 1)
    ui_rect_t (*position)(ui_iv_t* iv);
} ui_iv_if;

extern ui_iv_if ui_iv;

rt_end_c

