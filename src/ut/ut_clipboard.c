#include "ut/ut.h"
#include "ut/ut_win32.h"

static errno_t clipboard_put_text(const char* utf8) {
    errno_t r = 0;
    int32_t chars = str.utf16_chars(utf8);
    int32_t bytes = (chars + 1) * 2;
    wchar_t* utf16 = (wchar_t*)malloc(bytes);
    if (utf16 == null) {
        r = ERROR_OUTOFMEMORY;
    } else {
        str.utf8_utf16(utf16, utf8);
        assert(utf16[chars - 1] == 0);
        const int32_t n = (int)wcslen(utf16) + 1;
        r = OpenClipboard(GetDesktopWindow()) ? 0 : GetLastError();
        if (r != 0) { traceln("OpenClipboard() failed %s", str.error(r)); }
        if (r == 0) {
            r = EmptyClipboard() ? 0 : GetLastError();
            if (r != 0) { traceln("EmptyClipboard() failed %s", str.error(r)); }
        }
        void* global = null;
        if (r == 0) {
            global = GlobalAlloc(GMEM_MOVEABLE, n * 2);
            r = global != null ? 0 : GetLastError();
            if (r != 0) { traceln("GlobalAlloc() failed %s", str.error(r)); }
        }
        if (r == 0) {
            char* d = (char*)GlobalLock(global);
            not_null(d);
            memcpy(d, utf16, n * 2);
            r = SetClipboardData(CF_UNICODETEXT, global) ? 0 : GetLastError();
            GlobalUnlock(global);
            if (r != 0) {
                traceln("SetClipboardData() failed %s", str.error(r));
                GlobalFree(global);
            } else {
                // do not free global memory. It's owned by system clipboard now
            }
        }
        if (r == 0) {
            r = CloseClipboard() ? 0 : GetLastError();
            if (r != 0) {
                traceln("CloseClipboard() failed %s", str.error(r));
            }
        }
        free(utf16);
    }
    return r;
}

static errno_t clipboard_get_text(char* utf8, int32_t* bytes) {
    not_null(bytes);
    int r = OpenClipboard(GetDesktopWindow()) ? 0 : GetLastError();
    if (r != 0) { traceln("OpenClipboard() failed %s", str.error(r)); }
    if (r == 0) {
        HANDLE global = GetClipboardData(CF_UNICODETEXT);
        if (global == null) {
            r = GetLastError();
        } else {
            wchar_t* utf16 = (wchar_t*)GlobalLock(global);
            if (utf16 != null) {
                int32_t utf8_bytes = str.utf8_bytes(utf16);
                if (utf8 != null) {
                    char* decoded = (char*)malloc(utf8_bytes);
                    if (decoded == null) {
                        r = ERROR_OUTOFMEMORY;
                    } else {
                        str.utf16_utf8(decoded, utf16);
                        int32_t n = minimum(*bytes, utf8_bytes);
                        memcpy(utf8, decoded, n);
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
        r = CloseClipboard() ? 0 : GetLastError();
    }
    return r;
}

#ifdef UT_TESTS

static void clipboard_test(void) {
    fatal_if_not_zero(clipboard.put_text("Hello Clipboard"));
    char text[256];
    int32_t bytes = countof(text);
    fatal_if_not_zero(clipboard.get_text(text, &bytes));
    swear(strequ(text, "Hello Clipboard"));
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
}

#else

static void clipboard_test(void) {
}

#endif

clipboard_if clipboard = {
    .put_text   = clipboard_put_text,
    .get_text   = clipboard_get_text,
    .put_image  = null, // implemented in ui.app
    .test       = clipboard_test
};
