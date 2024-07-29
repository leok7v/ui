#include "ut/ut.h"
#include "ut/ut_win32.h"

static errno_t rt_clipboard_put_text(const char* utf8) {
    int32_t chars = ut_str.utf16_chars(utf8, -1);
    int32_t bytes = (chars + 1) * 2;
    uint16_t* utf16 = null;
    errno_t r = rt_heap.alloc((void**)&utf16, (size_t)bytes);
    if (utf16 != null) {
        ut_str.utf8to16(utf16, bytes, utf8, -1);
        ut_assert(utf16[chars - 1] == 0);
        const int32_t n = (int32_t)ut_str.len16(utf16) + 1;
        r = OpenClipboard(GetDesktopWindow()) ? 0 : rt_core.err();
        if (r != 0) { ut_println("OpenClipboard() failed %s", ut_strerr(r)); }
        if (r == 0) {
            r = EmptyClipboard() ? 0 : rt_core.err();
            if (r != 0) { ut_println("EmptyClipboard() failed %s", ut_strerr(r)); }
        }
        void* global = null;
        if (r == 0) {
            global = GlobalAlloc(GMEM_MOVEABLE, (size_t)n * 2);
            r = global != null ? 0 : rt_core.err();
            if (r != 0) { ut_println("GlobalAlloc() failed %s", ut_strerr(r)); }
        }
        if (r == 0) {
            char* d = (char*)GlobalLock(global);
            ut_not_null(d);
            memcpy(d, utf16, (size_t)n * 2);
            r = ut_b2e(SetClipboardData(CF_UNICODETEXT, global));
            GlobalUnlock(global);
            if (r != 0) {
                ut_println("SetClipboardData() failed %s", ut_strerr(r));
                GlobalFree(global);
            } else {
                // do not free global memory. It's owned by system clipboard now
            }
        }
        if (r == 0) {
            r = ut_b2e(CloseClipboard());
            if (r != 0) {
                ut_println("CloseClipboard() failed %s", ut_strerr(r));
            }
        }
        rt_heap.free(utf16);
    }
    return r;
}

static errno_t rt_clipboard_get_text(char* utf8, int32_t* bytes) {
    ut_not_null(bytes);
    errno_t r = ut_b2e(OpenClipboard(GetDesktopWindow()));
    if (r != 0) { ut_println("OpenClipboard() failed %s", ut_strerr(r)); }
    if (r == 0) {
        HANDLE global = GetClipboardData(CF_UNICODETEXT);
        if (global == null) {
            r = rt_core.err();
        } else {
            uint16_t* utf16 = (uint16_t*)GlobalLock(global);
            if (utf16 != null) {
                int32_t utf8_bytes = ut_str.utf8_bytes(utf16, -1);
                if (utf8 != null) {
                    char* decoded = (char*)malloc((size_t)utf8_bytes);
                    if (decoded == null) {
                        r = ERROR_OUTOFMEMORY;
                    } else {
                        ut_str.utf16to8(decoded, utf8_bytes, utf16, -1);
                        int32_t n = rt_min(*bytes, utf8_bytes);
                        memcpy(utf8, decoded, (size_t)n);
                        free(decoded);
                        if (n < utf8_bytes) {
                            r = ERROR_INSUFFICIENT_BUFFER;
                        }
                    }
                }
                *bytes = utf8_bytes;
                GlobalUnlock(global);
            }
        }
        r = ut_b2e(CloseClipboard());
    }
    return r;
}

#ifdef UT_TESTS

static void rt_clipboard_test(void) {
    rt_fatal_if_error(rt_clipboard.put_text("Hello Clipboard"));
    char text[256];
    int32_t bytes = ut_countof(text);
    rt_fatal_if_error(rt_clipboard.get_text(text, &bytes));
    rt_swear(strcmp(text, "Hello Clipboard") == 0);
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { ut_println("done"); }
}

#else

static void rt_clipboard_test(void) {
}

#endif

rt_clipboard_if rt_clipboard = {
    .put_text   = rt_clipboard_put_text,
    .get_text   = rt_clipboard_get_text,
    .put_image  = null, // implemented in ui.app
    .test       = rt_clipboard_test
};
