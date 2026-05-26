# ui

Single-header libraries for building primitive UI/UX on Windows 10/11.

[![build-on-push](https://github.com/leok7v/ui/actions/workflows/build-on-push.yml/badge.svg)](https://github.com/leok7v/ui/actions/workflows/build-on-push.yml)

## Namespaces

```
core_ / trace_  base types and fail-fast tracing
posix_          cross-platform fail-fast runtime (Win32 + POSIX), built on core/trace
ui_             UI types and interfaces (Windows-only)
```

## Layout

```
include/   public headers, prefixed:  core/core.h  trace/trace.h  posix/posix.h  ui/ui.h
src/       implementation:            core/  trace/  posix/  ui/
samples/   runnable demos (sfh, polyglot, translucent, mandrill, fractal,
           timers, groot, editor, guardians, layout, mandelbrot)
tools/     build tooling (amalgamate, version)
vendor/    third-party (stb)
```

## Getting started (Windows / Visual Studio 2022)

Open `msvc2022/ui.sln`, right-click any project under **samples/** (e.g. `layout` or
`polyglot`) and choose **Set as Startup Project**, then press **F5**.

From a developer command prompt:

```
msbuild msvc2022\ui.sln /p:Configuration=debug /p:Platform=x64 /m:1
```

## Minimal "Hello" application

```c
#include "posix/posix.h"
#include "ui/ui.h"

static ui_label_t label = ui_label(0.0, "Hello");

static void opened(void) {
    ui_view.add(ui_app.content, &label, null);
}

static void init(void) {
    ui_app.title  = "Sample";
    ui_app.opened = opened;
}

struct ui_app ui_app = {
    .class_name = "sample",
    .init = init,
    .window_sizing = {
        .ini_w = 4.0f, // 4x2 inches
        .ini_h = 2.0f
    }
};
```

## Samples and documentation

Every sample has a short, illustrated page in [docs/](docs/README.md): a
screenshot, what it demonstrates, the key code, and notes on its on-screen
layout. The pages are ordered from the simplest to the most involved, so
they double as a guided tour -- start at [sfh](docs/sfh.md) and follow the
Next links, or jump straight to the [index](docs/README.md).
