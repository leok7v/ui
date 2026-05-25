#include "rt/rt.h"
#include "ui/ui.h"
#include "rocket.h"
#include "groot.h"
#include "stb_image.h"

// TODO: stack(ui_text with the content "I am groot..", view_groot)
// Top: Find single line edit control "groot" with Find button that selects found text
// bottom: UTC time and local time status of all views?,
// Right view: debug toggles (text metric and margins) + message box on close window:
// "Groot: I am groot...\n"
// "Rocket: He says: Where the heck are you going now?\n"
// "Retry" "Abort" "Ignore"


enum { width = 512, height = 512 };

static uint8_t gs[width * height]; // greyscale
//static struct ui_bitmap image; // grayscale image

static struct ui_image view_groot;
static struct ui_image view_rocket;
static struct ui_image view_gs[2]; // two views at the same image

static struct ui_edit_view  view_text;
static struct ui_edit_doc   document;

static void* load_image(const uint8_t* data, int64_t bytes, int32_t* w, int32_t* h,
    int32_t* bpp, int32_t preferred_bytes_per_pixel) {
    void* pixels = stbi_load_from_memory((uint8_t const*)data, (int32_t)bytes, w, h,
        bpp, preferred_bytes_per_pixel);
    return pixels;
}

static void init_image(struct ui_bitmap* i, const uint8_t* data, int64_t bytes) {
    int32_t w = 0;
    int32_t h = 0;
    int32_t c = 0;
    void* pixels = load_image(data, bytes, &w, &h, &c, 0);
    rt_not_null(pixels);
    ui_draw.bitmap_init(i, w, h, c, pixels);
    stbi_image_free(pixels);
}

static void init_gs(void) {
    const struct ui_bitmap* i = &view_groot.image;
    uint32_t* pixels = (uint32_t*)i->pixels;
    rt_assert(i->w == 64 && i->h == 64 && i->bpp == 4);
    for (int y = 0; y < height; y++) {
        int32_t y64 = y % 64;
        for (int x = 0; x < width; x++) {
            int32_t x64 = x % 64;
            uint32_t rgba = pixels[y64 * 64 + x64];
            ui_color_t c = (ui_color_t)rgba;
            c = ui_colors.gray_with_same_intensity(c);
            gs[y * width + x] = ((x / 64) % 2) == ((y / 64) % 2) ?
                (uint8_t)(c & 0xFF) : 0xC0;
        }
    }
}

static void panel_erase(struct ui_view* v) {
    ui_draw.frame(v->x + 1, v->y + 1, v->w - 1, v->h - 1, ui_colors.black);
}

static void gs_erase(struct ui_view* v) {
    ui_draw.fill(v->x, v->y, v->w, v->h, ui_colors.ennui_black);
}

static void slider_format(struct ui_view* v) {
    struct ui_slider* s = (struct ui_slider*)v;
    ui_view.set_text(v, "%.0f%%", s->value * 100.0 / 255.0);
}

static void slider_callback(struct ui_view* v) {
    struct ui_slider* s = (struct ui_slider*)v;
    view_groot.alpha = (fp64_t)s->value / 256.0;
//  rt_println("value: %d", slider->value);
}

static void init_images(void) {
    ui_image.init(&view_groot);
    init_image(&view_groot.image, groot, rt_countof(groot));
    // view of groot image:
//  ui_image.ratio(&view_groot, 4, 1); // 4:1
    ui_image.ratio(&view_groot, 3, 1); // 4:1
    view_groot.alpha = 0.5;
    view_groot.padding = (struct ui_margins){0.125f, 0.125f, 0.125f, 0.125f};
    view_groot.focusable = false; // because it is stacked under text editor
    // view of rocket image:
    ui_image.init(&view_rocket);
    init_image(&view_rocket.image, rocket, rt_countof(rocket));
    ui_image.ratio(&view_rocket, 3, 1); // 3:1
    view_rocket.padding = (struct ui_margins){0.125f, 0.125f, 0.125f, 0.125f};
    view_groot.focusable = false; // no zoom/pan
    init_gs();
}

static void init_text(void) {
    rt_swear(ui_edit_doc.init(&document,
    "Star-Lord: \"What is wrong with giving tree, here?\"\n"
    "Rocket: \"Well, he don't know talking good like me and you, "
              "so his vocabulistics is limited to 'I' and 'am' and 'Groot.' "
              "Exclusively, in that order.\"", -1, false));
    ui_edit_view.init(&view_text, &document);
    view_text.hide_word_wrap = true;
view_text.hide_word_wrap = false; // TODO: debugging remove
    view_text.padding = (struct ui_margins){0};
// TODO: commented out for debugging uncomment is hiding word wrap
//  view_text.insets = (struct ui_margins){0};
    view_text.background_id = 0;
    view_text.background = ui_colors.transparent;
    rt_str_printf(view_text.hint,
        "Text Edit:\n\n"
        "Try double clicking to select a word\n"
        "or long-pressing to select a paragraph.\n\n"
        "Ctrl+[Shift]+arrows, Ctrl+X|C|V,\n"
        "Undo/Redo Ctrl+Z/Y\n"
    );
}

static struct ui_view* align(struct ui_view* v, int32_t align) {
    v->align = align;
    return v;
}

static struct ui_view* fill_parent(struct ui_view* v) {
    v->max_h  = ui.infinity;
    v->max_w  = ui.infinity;
    return v;
}

static void opened(void) {
    init_images();
    init_text();
    static struct ui_view  list         = ui_view(list);
    static ui_label_t label_left   = ui_label(0, "Left");
    static ui_label_t label_top    = ui_label(0, "Top");
    static ui_label_t label_bottom = ui_label(0, "Bottom");
    // painting greyscale pixels will be handled w/o device bitmap:
    for (int32_t i = 0; i < rt_countof(view_gs); i++) {
        ui_image.init_with(&view_gs[i], gs, width, height, 1, width);
        view_gs[i].erase = gs_erase;
        view_gs[i].focusable = true; // enable zoom pan
    }
    static struct ui_view   top    = ui_view(stack);
    static struct ui_view   center = ui_view(span);
    static struct ui_view   left   = ui_view(list);
    static struct ui_view   right  = ui_view(list);
    static struct ui_view   stack  = ui_view(stack);
    static struct ui_view   bottom = ui_view(stack);
    static struct ui_view   spacer = ui_view(spacer);
    static struct ui_slider slider = ui_slider("128", 16.0f, 0, 255,
            slider_format, slider_callback);
    slider.value = 128;
    ui_view.add(fill_parent(&left),
                align(&view_gs[0].view, ui.align.left),
                align(&view_gs[1].view, ui.align.left),
                null
    );
    ui_view.add(&stack,
                fill_parent(&view_groot.view),
                fill_parent(&view_text.view),
                null
    );
    ui_view.add(&right,
                &spacer,
                fill_parent(&view_rocket.view),
                &slider,
                fill_parent(&stack),
                null
    );
    ui_view.add(&top,    &label_top,    null);
    ui_view.add(&bottom, &label_bottom, null);
    ui_view.add(&center,
                align(&left,  ui.align.top),
                align(&right, ui.align.top),
                null);
    ui_view.add(ui_app.content,
        ui_view.add(fill_parent(&list),
                    align(&top,    ui.align.center),
                    align(&center, ui.align.left),
                    align(&bottom, ui.align.center),
                    null
        ),
        null
    );
    stack.debug.id = "#stack: edit+image";
    list.debug.id = "#list";
    right.debug.id = "#right";
//  list.debug.paint.margins = true;
right.debug.paint.margins = true;
    center.insets  = (struct ui_margins){0};
    center.padding = (struct ui_margins){0};
    static struct ui_view* panels[] = { &top, &left, &right, &bottom  };
    for (int32_t i = 0; i < rt_countof(panels); i++) {
        panels[i]->erase = panel_erase;
        panels[i]->padding = (struct ui_margins){0};
        panels[i]->insets  = (struct ui_margins){0.125f, 0.125f, 0.125f, 0.125f};
    }
    list.background_id = ui_color_id_window;
    ui_view_for_each(&list, it, {
//      it->debug.paint.margins = true;
        it->max_w   = ui.infinity;
    });
    ui_view_for_each(&center, it, {
//      it->debug.paint.margins = true;
        it->max_h   = ui.infinity;
    });
    center.max_h = ui.infinity;
    for (int32_t i = 0; i < rt_countof(view_gs); i++) {
        fill_parent(&view_gs[i].view)->erase = panel_erase;
    }
    view_gs[0].padding.bottom = 0.25f;
    view_gs[1].padding.top    = 0.25f;
    view_gs[0].fit  = true;
    view_gs[1].fill = true;
    view_groot.debug.id  = "#view.groot";
    view_rocket.debug.id = "#view.rocket";

    view_groot.fit = true;
    view_rocket.fit = true;

//  view_groot.fill = true;
//  view_rocket.fill = true;
}

static void closed(void) {
    ui_view.disband(ui_app.content);
    ui_draw.bitmap_dispose(&view_groot.image);
    ui_draw.bitmap_dispose(&view_rocket.image);
}

struct ui_app ui_app = {
    .class_name = "groot",
    .title = "Groot",
    .dark_mode = true,
    .opened = opened,
    .closed = closed,
    .window_sizing = { // inches
        .min_w = 5.0f,
        .min_h = 4.0f,
        .ini_w = 10.0f,
        .ini_h = 6.0f
    }
};
