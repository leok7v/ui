/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/ut/ut.h"
#include "single_file_lib/ui/ui.h"
#include "stb_image.h"

const char* title = "Sample4";

static ui_image_t image[2];

static char filename[260]; // c:\Users\user\Pictures\mandrill-4.2.03.png

static void init(void);

static int  console(void) {
    ut_fatal_if(true, "%s only SUBSYSTEM:WINDOWS", ut_args.basename());
    return 1;
}

ui_app_t ui_app = {
    .class_name = "sample4",
    .init = init,
    .dark_mode = true,
    .main = console,
    .window_sizing = {
        .min_w =  4.0f,
        .min_h =  4.0f,
        .ini_w =  6.0f,
        .ini_h =  6.0f
    }
};

static void* load_image(const uint8_t* data, int64_t bytes, int32_t* w, int32_t* h,
    int32_t* bpp, int32_t preferred_bytes_per_pixel);

static void load_images(void) {
    int r = 0;
    void* data = null;
    int64_t bytes = 0;
    for (int i = 0; i < ut_count_of(image); i++) {
        if (i == 0) {
            r = ut_mem.map_ro(filename, &data, &bytes);
        } else {
            r = ut_mem.map_resource("sample_png", &data, &bytes);
        }
        ut_fatal_if_error(r);
        int w = 0;
        int h = 0;
        int bpp = 0; // bytes (!) per pixel
        void* pixels = load_image(data, bytes, &w, &h, &bpp, 0);
        ut_not_null(pixels);
        ui_gdi.image_init(&image[i], w, h, bpp, pixels);
        stbi_image_free(pixels);
        // do not unmap resources:
        if (i == 0) { ut_mem.unmap(data, bytes); }
    }
}

static void paint(ui_view_t* view) {
    ui_gdi.fill(0, 0, view->w, view->h, ui_colors.black);
    if (image[1].w > 0 && image[1].h > 0) {
        int w = ut_min(view->w, image[1].w);
        int h = ut_min(view->h, image[1].h);
        int x = (view->w - w) / 2;
        int y = (view->h - h) / 2;
        ui_gdi.set_clip(0, 0, view->w, view->h);
        ui_gdi.image(x, y, w, h, &image[1]);
        ui_gdi.set_clip(0, 0, 0, 0);
    }
    if (image[0].w > 0 && image[0].h > 0) {
        int x = (view->w - image[0].w) / 2;
        int y = (view->h - image[0].h) / 2;
        ui_gdi.image(x, y, image[0].w, image[0].h, &image[0]);
    }
}

static void download(void) {
    static const char* url =
        "https://upload.wikimedia.org/wikipedia/commons/c/c1/"
        "Wikipedia-sipi-image-db-mandrill-4.2.03.png";
    if (!ut_files.exists(filename)) {
        char cmd[256];
        ut_str_printf(cmd, "curl.exe  --silent --fail --create-dirs "
            "\"%s\" --output \"%s\" 2>nul >nul", url, filename);
        int r = system(cmd);
        if (r != 0) {
            traceln("download %s failed %d %s", filename, r, strerr(r));
        }
    }
}

static void init(void) {
    ui_app.title = title;
    ui_app.content->paint = paint;
    ut_str_printf(filename, "%s\\mandrill-4.2.03.png",
        ut_files.known_folder(ut_files.folder.pictures));
    download();
    load_images();
}

static void* load_image(const uint8_t* data, int64_t bytes, int32_t* w, int32_t* h,
    int32_t* bpp, int32_t preferred_bytes_per_pixel) {
    void* pixels = stbi_load_from_memory((uint8_t const*)data, (int32_t)bytes, w, h,
        bpp, preferred_bytes_per_pixel);
    return pixels;
}
