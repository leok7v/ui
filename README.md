# ui

Single header libraries for building primitive UI/UX.

Win32 only implementation for now. 
Posix/MacOS/iOS/Linux/Android - possible

[![build-on-push](https://github.com/leok7v/ut/actions/workflows/build-on-push.yml/badge.svg)](https://github.com/leok7v/ut/actions/workflows/build-on-push.yml)

# Two Namespaces:

```
ut_ for utility helpers
ui_ for UI types and interfaces
```

# Minimalistic "Hello" Application Example

```c
#include "ui.h"
#include "ut.h"

static ui_label_t label = ui_label(0.0, "Hello");

static void opened(void) {
    ui_view.add(app.view, &label, null);
}

static void init(void) {
    app.title = "Sample";
    app.opened = opened;
}

app_t app = {
    .class_name = "sample",
    .init = init,
    .window_sizing = {
        .ini_w = 4.0f, // 4x2 inches
        .ini_h = 2.0f
    }
};
```
