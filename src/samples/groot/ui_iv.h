#include "ut/ut.h"
#include "ui/ui.h"

ut_begin_c

// "iv" stands for "image view"

typedef struct ui_iv_s ui_iv_t;

typedef struct ui_iv_s {
    union {
        ui_view_t view;
        struct ui_view_s;
    };
    struct {
        const uint8_t* pixels;
        int32_t w; // width
        int32_t h; // height
        int32_t c; // 1, 3, 4
        int32_t s; // stride - usually w * c but may differ
    } image;
    // actual zoom: z = 2 ^ (zn - 1) / 2 ^ (zd - 1)
    int32_t value; // 0..8
    // 0=16:1 1=8:1 2=4:1 3=2:1 4=1:1 5=1:2 6=1:4 7=1:8 8=1:16
    int32_t zn; // zoom nominator (1, 2, 3, ...)
    int32_t zd; // zoom denominator (1, 2, 3, ...)
    fp64_t  sx; // shift x [0..1.0] in view coordinates
    fp64_t  sy; // shift y [0..1.0]
    fp64_t    (*scale)(ui_iv_t* iv); // 2 ^ (zn - 1) / 2 ^ (zd - 1)
    ui_rect_t (*position)(ui_iv_t* iv);
    struct { // only visible when focused
        ui_view_t   bar; // ui_view(span) {zoom in, zoom 1:1, zoom out, help}
        ui_button_t copy; // copy image to clipboard
        ui_button_t zoom_in;
        ui_button_t zoom_1t1; // 1:1
        ui_button_t zoom_out;
        ui_button_t help;
        ui_label_t  ratio;
    } tool;
    ui_point_t drag_start;
    fp64_t when; // to hide toolbar
} ui_iv_t;

void ui_iv_init(ui_iv_t* iv);
void ui_iv_fini(ui_iv_t* iv);

ut_end_c

