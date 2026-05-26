# Samples

Each sample is a small, self-contained program built on the `ui` library
(which sits on the cross-platform `posix` runtime, plus `core` and `trace`).
The `ui` layer is Windows-only; `core`, `trace`, and `posix` also build on
Linux and macOS.

Every sample links the prebuilt `ui.lib` and compiles only its own `.c`
file. The one exception is `sfh`, which consumes the generated single-file
headers the stb way (see its page).

## A guided tour

The pages below are ordered from the simplest to the most involved. Read
top to bottom for a tour of the library, or jump to one that interests you;
each page has Prev / Next links to the neighbors in this order.

| #  | Sample       | What it adds to the tour                            |
| -- | ------------ | --------------------------------------------------- |
| 1  | sfh          | The smallest app, and how the code is assembled     |
| 2  | polyglot     | A label, a periodic callback, locale switching      |
| 3  | translucent  | A borderless, translucent window with a custom caption |
| 4  | mandrill     | Loading and drawing images (stb_image)              |
| 5  | fractal      | A background render thread and double buffering     |
| 6  | timers       | Window timers vs a realtime thread, drawn as graphs |
| 7  | groot        | Animated GIFs and looping MIDI                       |
| 8  | editor       | The text editing controls                           |
| 9  | guardians    | Several views composed over one image set           |
| 10 | layout       | The layout engine: containers, insets, alignment    |
| 11 | mandelbrot   | A full app: zoom, pan, i18n, dialogs, clipboard     |

## How to build and run

- Open `msvc2022/ui.sln` in Visual Studio 2022.
- Right-click a sample project -> "Set as Startup Project".
- Press F5 (Debug) or Ctrl+F5 (Run).

Or run a prebuilt executable directly from `bin\<config>\<platform>\`, for
example `bin\debug\x64\layout.exe`.

## Pages

1. [sfh](sfh.md)
2. [polyglot](polyglot.md)
3. [translucent](translucent.md)
4. [mandrill](mandrill.md)
5. [fractal](fractal.md)
6. [timers](timers.md)
7. [groot](groot.md)
8. [editor](editor.md)
9. [guardians](guardians.md)
10. [layout](layout.md)
11. [mandelbrot](mandelbrot.md)
