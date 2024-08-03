# ui

Single header libraries for building primitive UI/UX.

Win32 only implementation for now. 
Posix/MacOS/iOS/Linux/Android - possible

[![build-on-push](https://github.com/leok7v/ut/actions/workflows/build-on-push.yml/badge.svg)](https://github.com/leok7v/ut/actions/workflows/build-on-push.yml)

# Two Namespaces:

```
rt_ for minimalistic fail fast runtime
ui_ for UI types and interfaces
```

# Minimalistic "Hello" Application Example

```c
#include "ui/ui.h"
#include "rt/rt.h"

static ui_label_t label = ui_label(0.0, "Hello");

static void opened(void) {
    ui_view.add(ui_app.view, &label, null);
}

static void init(void) {
    ui_app.title = "Sample";
    ui_app.opened = opened;
}

ui_app_t ui_app = {
    .class_name = "sample",
    .init = init,
    .window_sizing = {
        .ini_w = 4.0f, // 4x2 inches
        .ini_h = 2.0f
    }
};
```
