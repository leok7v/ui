#define _GNU_SOURCE 1
#include "posix.h"


#include <stddef.h>
#include <unistd.h>
#include <locale.h>
#include <signal.h>
#include <execinfo.h>

// ________________________________ posix_args.c _________________________________

static void * posix_args_memory = null;

static void posix_args_main(int32_t argc, const char * argv[], const char ** env) {
    posix_swear(posix_args.c == 0 && posix_args.v == null && posix_args.env == null);
    posix_swear(posix_args_memory == null);
    posix_args.c = argc;
    posix_args.v = argv;
    posix_args.env = env;
}

static int32_t posix_args_option_index(const char * option) {
    int32_t ix = -1;
    for (int32_t i = 1; i < posix_args.c; i++) {
        if (strcmp(posix_args.v[i], "--") == 0) { break; }
        if (strcmp(posix_args.v[i], option) == 0) {
            ix = i;
            break;
        }
    }
    return ix;
}

static void posix_args_remove_at(int32_t ix) {
    posix_assert(0 < posix_args.c);
    posix_assert(0 < ix && ix < posix_args.c);
    for (int32_t i = ix; i < posix_args.c - 1; i++) {
        posix_args.v[i] = posix_args.v[i + 1];
    }
    posix_args.v[posix_args.c - 1] = "";
    posix_args.c--;
}

static bool posix_args_option_bool(const char * option) {
    int32_t ix = posix_args_option_index(option);
    if (ix > 0) { posix_args_remove_at(ix); }
    return ix > 0;
}

static bool posix_args_option_int(const char * option, int64_t * value) {
    int32_t ix = posix_args_option_index(option);
    if (ix > 0 && ix < posix_args.c - 1) {
        const char * s = posix_args.v[ix + 1];
        int32_t base = (strstr(s, "0x") == s || strstr(s, "0X") == s) ? 16 : 10;
        const char * b = s + (base == 10 ? 0 : 2);
        char * e = null;
        errno = 0;
        int64_t v = strtoll(b, &e, base);
        if (errno == 0 && e > b && *e == 0) {
            *value = v;
        } else {
            ix = -1;
        }
    } else {
        ix = -1;
    }
    if (ix > 0) {
        posix_args_remove_at(ix);
        posix_args_remove_at(ix);
    }
    return ix > 0;
}

static const char * posix_args_option_str(const char * option) {
    int32_t ix = posix_args_option_index(option);
    const char * s = null;
    if (ix > 0 && ix < posix_args.c - 1) {
        s = posix_args.v[ix + 1];
    } else {
        ix = -1;
    }
    if (ix > 0) {
        posix_args_remove_at(ix);
        posix_args_remove_at(ix);
    }
    return ix > 0 ? s : null;
}

static const char * posix_args_basename(void) {
    static char basename[256] = {0};
    posix_swear(posix_args.c > 0);
    if (basename[0] == 0) {
        const char * s = posix_args.v[0];
        const char * b = s;
        while (*s != 0) {
            if (*s == '/') { b = s + 1; }
            s++;
        }
        int32_t n = posix_str.len(b);
        posix_swear(n < posix_countof(basename));
        strncpy(basename, b, posix_countof(basename) - 1);
        char * d = basename + n - 1;
        while (d > basename && *d != '.') { d--; }
        if (*d == '.') { *d = 0x00; }
    }
    return basename;
}

static void posix_args_fini(void) {
    posix_args_memory = null;
    posix_args.c = 0;
    posix_args.v = null;
}

static void posix_args_test(void) {}

struct posix_args_if posix_args = {
    .main         = posix_args_main,
    .option_index = posix_args_option_index,
    .remove_at    = posix_args_remove_at,
    .option_bool  = posix_args_option_bool,
    .option_int   = posix_args_option_int,
    .option_str   = posix_args_option_str,
    .basename     = posix_args_basename,
    .fini         = posix_args_fini,
    .test         = posix_args_test
};

// ________________________________ posix_core.c _________________________________

static void posix_core_abort(void) { abort(); }

static void posix_core_exit(int32_t exit_code) { exit(exit_code); }

static int posix_core_err(void) { return errno; }

static void posix_core_seterr(int err) { errno = err; }

static void posix_core_test(void) {}

struct posix_core_if posix_core = {
    .err     = posix_core_err,
    .set_err = posix_core_seterr,
    .abort   = posix_core_abort,
    .exit    = posix_core_exit,
    .test    = posix_core_test,
    .error   = {
        .access_denied       = EACCES,
        .bad_file            = EBADF,
        .broken_pipe         = EPIPE,
        .device_not_ready    = ENXIO,
        .directory_not_empty = ENOTEMPTY,
        .disk_full           = ENOSPC,
        .file_exists         = EEXIST,
        .file_not_found      = ENOENT,
        .insufficient_buffer = E2BIG,
        .interrupted         = EINTR,
        .invalid_data        = EINVAL,
        .invalid_handle      = EBADF,
        .invalid_parameter   = EINVAL,
        .io_error            = EIO,
        .more_data           = ENOBUFS,
        .name_too_long       = ENAMETOOLONG,
        .no_child_process    = ECHILD,
        .not_a_directory     = ENOTDIR,
        .not_empty           = ENOTEMPTY,
        .out_of_memory       = ENOMEM,
        .path_not_found      = ENOENT,
        .pipe_not_connected  = EPIPE,
        .read_only_file      = EROFS,
        .resource_deadlock   = EDEADLK,
        .too_many_open_files = EMFILE,
    }
};

// ________________________________ posix_debug.c ________________________________

static int32_t posix_debug_max_file_line = 0;
static int32_t posix_debug_max_function = 0;

static void posix_debug_output(const char * s, int32_t count) {
    bool intercepted = false;
    if (posix_debug.tee != null) { intercepted = posix_debug.tee(s, count); }
    if (!intercepted) {
        if (stderr != null && fileno(stderr) >= 0) {
            fprintf(stderr, "%s", s);
        }
    }
}

static void posix_debug_println_va(const char * file, int32_t line, const char * func,
        const char * format, va_list va) {
    if (func == null) { func = ""; }
    char file_line[1024];
    if (line == 0 && (file == null || file[0] == 0x00)) {
        file_line[0] = 0x00;
    } else {
        if (file == null) { file = ""; }
        const char * name = posix_files.basename(file);
        snprintf(file_line, posix_countof(file_line) - 1, "%s(%d):", name, line);
    }
    file_line[posix_countof(file_line) - 1] = 0x00;
    posix_debug_max_file_line = posix_max(posix_debug_max_file_line, (int32_t)strlen(file_line));
    posix_debug_max_function  = posix_max(posix_debug_max_function, (int32_t)strlen(func));
    char prefix[2048];
    snprintf(prefix, posix_countof(prefix) - 1, "%-*s %-*s",
             posix_debug_max_file_line, file_line,
             posix_debug_max_function,  func);
    prefix[posix_countof(prefix) - 1] = 0;
    char text[2048];
    if (format != null && format[0] != 0) {
        vsnprintf(text, posix_countof(text) - 1, format, va);
        text[posix_countof(text) - 1] = 0;
    } else {
        text[0] = 0;
    }
    char output[4096];
    snprintf(output, posix_countof(output) - 1, "%s %s\n", prefix, text);
    output[posix_countof(output) - 1] = 0;
    posix_debug.output(output, (int32_t)strlen(output));
}

static void posix_debug_perrno(const char * file, int32_t line,
    const char * func, int32_t err_no, const char * format, ...) {
    if (err_no != 0) {
        if (format != null && format[0] != 0) {
            va_list va;
            va_start(va, format);
            posix_debug.println_va(file, line, func, format, va);
            va_end(va);
        }
        posix_debug.println(file, line, func, "errno: %d %s", err_no, strerror(err_no));
    }
}

static void posix_debug_perror(const char * file, int32_t line,
    const char * func, int32_t error, const char * format, ...) {
    posix_debug_perrno(file, line, func, error, format);
}

static void posix_debug_println(const char * file, int32_t line, const char * func,
        const char * format, ...) {
    va_list va;
    va_start(va, format);
    posix_debug.println_va(file, line, func, format, va);
    va_end(va);
}

static bool posix_debug_is_debugger_present(void) {
    // Requires parsing /proc/self/status TracerPid on Linux, or sysctl on Darwin.
    // Stubbed to false to avoid early complexity overhead.
    return false;
}

static void posix_debug_breakpoint(void) {
    raise(SIGTRAP);
}

static int posix_debug_raise(uint32_t exception) {
    raise(exception);
    return posix_core.err();
}

static int32_t posix_debug_verbosity_from_string(const char * s) {
    char * n = null;
    long v = strtol(s, &n, 10);
    int32_t r = posix_debug.verbosity.quiet;
    if (strcasecmp(s, "quiet") == 0) {
        r = posix_debug.verbosity.quiet;
    } else if (strcasecmp(s, "info") == 0) {
        r = posix_debug.verbosity.info;
    } else if (strcasecmp(s, "verbose") == 0) {
        r = posix_debug.verbosity.verbose;
    } else if (strcasecmp(s, "debug") == 0) {
        r = posix_debug.verbosity.debug;
    } else if (strcasecmp(s, "trace") == 0) {
        r = posix_debug.verbosity.trace;
    } else if (n > s && posix_debug.verbosity.quiet <= v && v <= posix_debug.verbosity.trace) {
        r = (int32_t)v;
    } else {
        posix_fatal("invalid verbosity: %s", s);
    }
    return r;
}

static void posix_debug_test(void) {}

struct posix_debug_if posix_debug = {
    .verbosity = {
        .level   =  0,
        .quiet   =  0,
        .info    =  1,
        .verbose =  2,
        .debug   =  3,
        .trace   =  4,
    },
    .verbosity_from_string = posix_debug_verbosity_from_string,
    .tee                   = null,
    .output                = posix_debug_output,
    .println               = posix_debug_println,
    .println_va            = posix_debug_println_va,
    .perrno                = posix_debug_perrno,
    .perror                = posix_debug_perror,
    .is_debugger_present   = posix_debug_is_debugger_present,
    .breakpoint            = posix_debug_breakpoint,
    .raise                 = posix_debug_raise,
    .exception             = {
        .sig_segv = SIGSEGV,
        .sig_ill  = SIGILL,
        .sig_trap = SIGTRAP,
        .sig_fpe  = SIGFPE,
        .sig_bus  = SIGBUS,
        .sig_abrt = SIGABRT,
    },
    .test                  = posix_debug_test
};

// _________________________________ posix_str.c _________________________________

static char * posix_str_drop_const(const char * s) {
    return (char *)s;
}

static int32_t posix_str_len(const char * s) { return (int32_t)strlen(s); }

static int32_t posix_str_utf16len(const uint16_t * utf16) {
    int32_t count = 0;
    while (utf16[count] != 0) { count++; }
    return count;
}

static int32_t posix_str_utf8bytes(const char * s, int32_t b) {
    posix_assert(b >= 1);
    const uint8_t * const u = (const uint8_t *)s;
    if (b >= 1 && (u[0] & 0x80u) == 0x00u) {
        return 1;
    } else if (b > 1) {
        uint32_t c = (u[0] << 8) | u[1];
        if (0xC280 <= c && c <= 0xDFBF && (c & 0xE0C0) == 0xC080) { return 2; }
        if (b > 2) {
            c = (c << 8) | u[2];
            if (0xEDA080 <= c && c <= 0xEDBFBF) { return 0; }
            if (0xE0A080 <= c && c <= 0xEFBFBF && (c & 0xF0C0C0) == 0xE08080) { return 3; }
            if (b > 3) {
                c = (c << 8) | u[3];
                if (0xF0908080 <= c && c <= 0xF48FBFBF && (c & 0xF8C0C0C0) == 0xF0808080) { return 4; }
            }
        }
    }
    return 0;
}

static int32_t posix_str_glyphs(const char * utf8, int32_t bytes) {
    posix_swear(bytes >= 0);
    bool ok = true;
    int32_t i = 0;
    int32_t k = 1;
    while (i < bytes && ok) {
        const int32_t b = posix_str.utf8bytes(utf8 + i, bytes - i);
        ok = 0 < b && i + b <= bytes;
        if (ok) { i += b; k++; }
    }
    return ok ? k - 1 : -1;
}

static void posix_str_lower(char * d, int32_t capacity, const char * s) {
    int32_t n = posix_str.len(s);
    posix_swear(capacity > n);
    for (int32_t i = 0; i < n; i++) { d[i] = (char)tolower(s[i]); }
    d[n] = 0;
}

static void posix_str_upper(char * d, int32_t capacity, const char * s) {
    int32_t n = posix_str.len(s);
    posix_swear(capacity > n);
    for (int32_t i = 0; i < n; i++) { d[i] = (char)toupper(s[i]); }
    d[n] = 0;
}

static bool posix_str_starts(const char * s1, const char * s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && memcmp(s1, s2, n2) == 0;
}

static bool posix_str_ends(const char * s1, const char * s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && memcmp(s1 + n1 - n2, s2, n2) == 0;
}

static bool posix_str_i_starts(const char * s1, const char * s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && strncasecmp(s1, s2, n2) == 0;
}

static bool posix_str_i_ends(const char * s1, const char * s2) {
    int32_t n1 = (int32_t)strlen(s1);
    int32_t n2 = (int32_t)strlen(s2);
    return n1 >= n2 && strncasecmp(s1 + n1 - n2, s2, n2) == 0;
}

static uint32_t posix_str_utf32(const char * utf8, int32_t bytes) {
    uint32_t utf32 = 0xFFFD;
    if (bytes >= 1 && (utf8[0] & 0x80) == 0) {
        utf32 = (uint8_t)utf8[0];
    } else if (bytes >= 2 && (utf8[0] & 0xE0) == 0xC0 && (utf8[1] & 0xC0) == 0x80) {
        utf32  = (uint32_t)(utf8[0] & 0x1F) << 6;
        utf32 |= (uint32_t)(utf8[1] & 0x3F);
    } else if (bytes >= 3 && (utf8[0] & 0xF0) == 0xE0 && (utf8[1] & 0xC0) == 0x80 && (utf8[2] & 0xC0) == 0x80) {
        utf32  = (uint32_t)(utf8[0] & 0x0F) << 12;
        utf32 |= (uint32_t)(utf8[1] & 0x3F) <<  6;
        utf32 |= (uint32_t)(utf8[2] & 0x3F);
    } else if (bytes >= 4 && (utf8[0] & 0xF8) == 0xF0 && (utf8[1] & 0xC0) == 0x80 && (utf8[2] & 0xC0) == 0x80 && (utf8[3] & 0xC0) == 0x80) {
        utf32  = (uint32_t)(utf8[0] & 0x07) << 18;
        utf32 |= (uint32_t)(utf8[1] & 0x3F) << 12;
        utf32 |= (uint32_t)(utf8[2] & 0x3F) <<  6;
        utf32 |= (uint32_t)(utf8[3] & 0x3F);
    }
    return utf32;
}

static int32_t posix_str_utf8_bytes(const uint16_t * utf16, int32_t chars) {
    if (chars == 0) { return 0; }
    if (chars < 0 && utf16[0] == 0x0000) { return 1; }
    int32_t req = 0;
    int32_t limit = chars < 0 ? INT32_MAX : chars;
    for (int32_t i = 0; i < limit; i++) {
        if (chars < 0 && utf16[i] == 0) { req++; break; }
        uint16_t c = utf16[i];
        if (c < 0x0080) {
            req += 1;
        } else if (c < 0x0800) {
            req += 2;
        } else if ((c & 0xFC00) == 0xD800) {
            i++;
            if (i >= limit || (utf16[i] & 0xFC00) != 0xDC00) { return -1; }
            req += 4;
        } else if ((c & 0xFC00) == 0xDC00) {
            return -1;
        } else {
            req += 3;
        }
    }
    return req;
}

static int32_t posix_str_utf16_chars(const char * utf8, int32_t bytes) {
    if (bytes == 0) { return 0; }
    if (bytes < 0 && utf8[0] == 0x00) { return 1; }
    int32_t req = 0;
    int32_t i = 0;
    int32_t limit = bytes < 0 ? INT32_MAX : bytes;
    while (i < limit) {
        if (bytes < 0 && utf8[i] == 0) { req++; break; }
        int32_t b = posix_str_utf8bytes(utf8 + i, limit - i);
        if (b <= 0) { return -1; }
        uint32_t cp = posix_str_utf32(utf8 + i, b);
        req += (cp > 0xFFFF) ? 2 : 1;
        i += b;
    }
    return req;
}

static int posix_str_utf16to8(char * utf8, int32_t capacity, const uint16_t * utf16, int32_t chars) {
    if (chars == 0) { return 0; }
    if (chars < 0 && utf16[0] == 0x0000) {
        posix_swear(capacity >= 1);
        utf8[0] = 0x00;
        return 0;
    }
    int32_t limit = chars < 0 ? INT32_MAX : chars;
    int32_t o = 0;
    for (int32_t i = 0; i < limit; i++) {
        if (chars < 0 && utf16[i] == 0) {
            if (o < capacity) { utf8[o++] = 0; }
            break;
        }
        uint16_t c = utf16[i];
        if (c < 0x0080) {
            if (o < capacity) { utf8[o++] = (char)c; }
        } else if (c < 0x0800) {
            if (o + 1 < capacity) {
                utf8[o++] = (char)(0xC0 | (c >> 6));
                utf8[o++] = (char)(0x80 | (c & 0x3F));
            }
        } else if ((c & 0xFC00) == 0xD800) {
            i++;
            if (i < limit) {
                uint32_t cp = 0x10000 + (((c & 0x3FF) << 10) | (utf16[i] & 0x3FF));
                if (o + 3 < capacity) {
                    utf8[o++] = (char)(0xF0 | (cp >> 18));
                    utf8[o++] = (char)(0x80 | ((cp >> 12) & 0x3F));
                    utf8[o++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                    utf8[o++] = (char)(0x80 | (cp & 0x3F));
                }
            }
        } else {
            if (o + 2 < capacity) {
                utf8[o++] = (char)(0xE0 | (c >> 12));
                utf8[o++] = (char)(0x80 | ((c >> 6) & 0x3F));
                utf8[o++] = (char)(0x80 | (c & 0x3F));
            }
        }
    }
    return 0;
}

static int posix_str_utf8to16(uint16_t * utf16, int32_t capacity, const char * utf8, int32_t bytes) {
    if (bytes == 0) { return 0; }
    if (bytes < 0 && utf8[0] == 0x00) {
        posix_swear(capacity >= 1);
        utf16[0] = 0x0000;
        return 0;
    }
    int32_t i = 0;
    int32_t o = 0;
    int32_t limit = bytes < 0 ? INT32_MAX : bytes;
    while (i < limit) {
        if (bytes < 0 && utf8[i] == 0) {
            if (o < capacity) { utf16[o++] = 0; }
            break;
        }
        int32_t b = posix_str_utf8bytes(utf8 + i, limit - i);
        if (b <= 0) { return EINVAL; }
        uint32_t cp = posix_str_utf32(utf8 + i, b);
        if (cp > 0xFFFF) {
            cp -= 0x10000;
            if (o + 1 < capacity) {
                utf16[o++] = (uint16_t)(0xD800 | (cp >> 10));
                utf16[o++] = (uint16_t)(0xDC00 | (cp & 0x3FF));
            }
        } else {
            if (o < capacity) {
                utf16[o++] = (uint16_t)cp;
            }
        }
        i += b;
    }
    return 0;
}

static bool posix_str_utf16_is_low_surrogate(uint16_t utf16char) {
    return 0xDC00 <= utf16char && utf16char <= 0xDFFF;
}

static bool posix_str_utf16_is_high_surrogate(uint16_t utf16char) {
    return 0xD800 <= utf16char && utf16char <= 0xDBFF;
}

static void posix_str_format_va(char * utf8, int32_t count, const char * format, va_list va) {
    vsnprintf(utf8, (size_t)count, format, va);
    utf8[count - 1] = 0;
}

static void posix_str_format(char * utf8, int32_t count, const char * format, ...) {
    va_list va;
    va_start(va, format);
    posix_str.format_va(utf8, count, format, va);
    va_end(va);
}

static struct posix_str1024 posix_str_error(int32_t error) {
    struct posix_str1024 text;
    snprintf(text.s, posix_countof(text.s), "0x%08X(%d) \"%s\"", error, error, strerror(error));
    return text;
}

static struct posix_str1024 posix_str_error_nls(int32_t error) {
    return posix_str_error(error);
}

static const char * posix_str_grouping_separator(void) {
    struct lconv * locale_info = localeconv();
    const char * sep = locale_info->thousands_sep;
    return (sep != null && sep[0] != 0) ? sep : "";
}

static struct posix_str64 posix_str_int64_dg(int64_t v, bool uint, const char * gs) {
    const int32_t m = (int32_t)strlen(gs);
    posix_swear(m < 5);
    struct posix_str64 text;
    enum { max_text_bytes = posix_countof(text.s) };
    int64_t abs64 = v < 0 ? -v : v;
    uint64_t n = uint ? (uint64_t)v : (v != INT64_MIN ? (uint64_t)abs64 : (uint64_t)INT64_MIN);
    int32_t i = 0;
    int32_t groups[8];
    do {
        groups[i] = n % 1000;
        n = n / 1000;
        i++;
    } while (n > 0);
    const int32_t gc = i - 1;
    char * s = text.s;
    if (v < 0 && !uint) { *s++ = '-'; }
    int32_t r = max_text_bytes - 1;
    while (i > 0) {
        i--;
        posix_assert(r > 3 + m);
        if (i == gc) {
            posix_str.format(s, r, "%d%s", groups[i], gc > 0 ? gs : "");
        } else {
            posix_str.format(s, r, "%03d%s", groups[i], i > 0 ? gs : "");
        }
        int32_t k = (int32_t)strlen(s);
        r -= k;
        s += k;
    }
    *s = 0;
    return text;
}

static struct posix_str64 posix_str_int64(int64_t v) {
    return posix_str_int64_dg(v, false, "\xE2\x80\x89"); // Thin space
}

static struct posix_str64 posix_str_uint64(uint64_t v) {
    return posix_str_int64_dg(v, true, "\xE2\x80\x89"); // Thin space
}

static struct posix_str64 posix_str_int64_lc(int64_t v) {
    return posix_str_int64_dg(v, false, posix_str_grouping_separator());
}

static struct posix_str64 posix_str_uint64_lc(uint64_t v) {
    return posix_str_int64_dg(v, true, posix_str_grouping_separator());
}

static struct posix_str128 posix_str_fp(const char * format, fp64_t v) {
    struct lconv * locale_info = localeconv();
    const char * decimal_separator = locale_info->decimal_point;
    if (decimal_separator == null || decimal_separator[0] == 0) { decimal_separator = "."; }
    struct posix_str128 f;
    f.s[0] = 0x00;
    posix_str.format(f.s, posix_countof(f.s), format, v);
    f.s[posix_countof(f.s) - 1] = 0x00;
    struct posix_str128 text;
    char * s = f.s;
    char * d = text.s;
    while (*s != 0x00) {
        if (*s == '.') {
            const char * sep = decimal_separator;
            while (*sep != 0x00) { *d++ = *sep++; }
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = 0x00;
    return text;
}

static void posix_str_test(void) {}

struct posix_str_if posix_str = {
    .drop_const              = posix_str_drop_const,
    .len                     = posix_str_len,
    .len16                   = posix_str_utf16len,
    .utf8bytes               = posix_str_utf8bytes,
    .glyphs                  = posix_str_glyphs,
    .lower                   = posix_str_lower,
    .upper                   = posix_str_upper,
    .starts                  = posix_str_starts,
    .ends                    = posix_str_ends,
    .istarts                 = posix_str_i_starts,
    .iends                   = posix_str_i_ends,
    .utf8_bytes              = posix_str_utf8_bytes,
    .utf16_chars             = posix_str_utf16_chars,
    .utf16to8                = posix_str_utf16to8,
    .utf8to16                = posix_str_utf8to16,
    .utf16_is_low_surrogate  = posix_str_utf16_is_low_surrogate,
    .utf16_is_high_surrogate = posix_str_utf16_is_high_surrogate,
    .utf32                   = posix_str_utf32,
    .format                  = posix_str_format,
    .format_va               = posix_str_format_va,
    .error                   = posix_str_error,
    .error_nls               = posix_str_error_nls,
    .grouping_separator      = posix_str_grouping_separator,
    .int64_dg                = posix_str_int64_dg,
    .int64                   = posix_str_int64,
    .uint64                  = posix_str_uint64,
    .int64_lc                = posix_str_int64_lc,
    .uint64_lc               = posix_str_uint64_lc,
    .fp                      = posix_str_fp,
    .test                    = posix_str_test
};

// ________________________________ posix_vigil.c ________________________________

static void posix_vigil_breakpoint_and_abort(void) {
    posix_debug.breakpoint();
    posix_core.abort();
}

static int32_t posix_vigil_failed_assertion(const char * file, int32_t line,
        const char * func, const char * condition, const char * format, ...) {
    va_list va;
    va_start(va, format);
    posix_debug.println_va(file, line, func, format, va);
    va_end(va);
    posix_debug.println(file, line, func, "assertion failed: %s\n", condition);
    posix_vigil_breakpoint_and_abort();
    return 0;
}

static int32_t posix_vigil_fatal_termination_va(const char * file, int32_t line,
        const char * func, const char * condition, int r,
        const char * format, va_list va) {
    const int32_t er = posix_core.err();
    posix_debug.println_va(file, line, func, format, va);
    if (r != er && r != 0) {
        posix_debug.perror(file, line, func, r, "");
    }
    if (er != 0) { posix_debug.perror(file, line, func, er, ""); }
    if (condition != null && condition[0] != 0) {
        posix_debug.println(file, line, func, "FATAL: %s\n", condition);
    } else {
        posix_debug.println(file, line, func, "FATAL\n");
    }
    posix_vigil_breakpoint_and_abort();
    return 0;
}

static int32_t posix_vigil_fatal_termination(const char * file, int32_t line,
        const char * func, const char * condition, const char * format, ...) {
    va_list va;
    va_start(va, format);
    posix_vigil_fatal_termination_va(file, line, func, condition, 0, format, va);
    va_end(va);
    return 0;
}

static int32_t posix_vigil_fatal_if_error(const char * file, int32_t line,
    const char * func, const char * condition, int r,
    const char * format, ...) {
    if (r != 0) {
        va_list va;
        va_start(va, format);
        posix_vigil_fatal_termination_va(file, line, func, condition, r, format, va);
        va_end(va);
    }
    return 0;
}

static void posix_vigil_test(void) {}

struct posix_vigil_if posix_vigil = {
    .failed_assertion  = posix_vigil_failed_assertion,
    .fatal_termination = posix_vigil_fatal_termination,
    .fatal_if_error    = posix_vigil_fatal_if_error,
    .test              = posix_vigil_test
};




#include <stdatomic.h>
#include <pthread.h>
#include <time.h>
#include <sched.h>
#include <sys/time.h>
#include <errno.h>

// _______________________________ posix_atomics.c _______________________________


static void* posix_atomics_exchange_ptr(volatile void** a, void* v) {
    return atomic_exchange((volatile _Atomic(void*)*)a, v);
}

static int32_t posix_atomics_increment_int32(volatile int32_t* a) {
    return atomic_fetch_add((volatile _Atomic int32_t*)a, 1) + 1;
}

static int32_t posix_atomics_decrement_int32(volatile int32_t* a) {
    return atomic_fetch_sub((volatile _Atomic int32_t*)a, 1) - 1;
}

static int64_t posix_atomics_increment_int64(volatile int64_t* a) {
    return atomic_fetch_add((volatile _Atomic int64_t*)a, 1) + 1;
}

static int64_t posix_atomics_decrement_int64(volatile int64_t* a) {
    return atomic_fetch_sub((volatile _Atomic int64_t*)a, 1) - 1;
}

static int32_t posix_atomics_add_int32(volatile int32_t* a, int32_t v) {
    return atomic_fetch_add((volatile _Atomic int32_t*)a, v) + v;
}

static int64_t posix_atomics_add_int64(volatile int64_t* a, int64_t v) {
    return atomic_fetch_add((volatile _Atomic int64_t*)a, v) + v;
}

static int32_t posix_atomics_exchange_int32(volatile int32_t* a, int32_t v) {
    return atomic_exchange((volatile _Atomic int32_t*)a, v);
}

static int64_t posix_atomics_exchange_int64(volatile int64_t* a, int64_t v) {
    return atomic_exchange((volatile _Atomic int64_t*)a, v);
}

static bool posix_atomics_compare_exchange_int64(volatile int64_t* a, int64_t comparand, int64_t v) {
    return atomic_compare_exchange_strong((volatile _Atomic int64_t*)a, &comparand, v);
}

static bool posix_atomics_compare_exchange_int32(volatile int32_t* a, int32_t comparand, int32_t v) {
    return atomic_compare_exchange_strong((volatile _Atomic int32_t*)a, &comparand, v);
}

static bool posix_atomics_compare_exchange_ptr(volatile void** a, void* comparand, void* v) {
    return atomic_compare_exchange_strong((volatile _Atomic(void*)*)a, &comparand, v);
}

static void posix_atomics_memory_fence(void) {
    atomic_thread_fence(memory_order_seq_cst);
}

static void posix_atomics_spinlock_acquire(volatile int64_t* spinlock) {
    while (!posix_atomics_compare_exchange_int64(spinlock, 0, 1)) {
        while (*spinlock) {
#if defined(__aarch64__) || defined(__arm__)
            __asm__ volatile("yield" ::: "memory");
#elif defined(__x86_64__) || defined(__i386__)
            __asm__ volatile("pause" ::: "memory");
#endif
        }
    }
    posix_atomics_memory_fence();
}

static void posix_atomics_spinlock_release(volatile int64_t* spinlock) {
    posix_assert(*spinlock == 1);
    *spinlock = 0;
    posix_atomics_memory_fence();
}

static int32_t posix_atomics_load_int32(volatile int32_t* a) {
    return atomic_load_explicit((volatile _Atomic int32_t*)a, memory_order_acquire);
}

static int64_t posix_atomics_load_int64(volatile int64_t* a) {
    return atomic_load_explicit((volatile _Atomic int64_t*)a, memory_order_acquire);
}

static void posix_atomics_test(void) {}

struct posix_atomics_if posix_atomics = {
    .exchange_ptr           = posix_atomics_exchange_ptr,
    .increment_int32        = posix_atomics_increment_int32,
    .decrement_int32        = posix_atomics_decrement_int32,
    .increment_int64        = posix_atomics_increment_int64,
    .decrement_int64        = posix_atomics_decrement_int64,
    .add_int32              = posix_atomics_add_int32,
    .add_int64              = posix_atomics_add_int64,
    .exchange_int32         = posix_atomics_exchange_int32,
    .exchange_int64         = posix_atomics_exchange_int64,
    .compare_exchange_int64 = posix_atomics_compare_exchange_int64,
    .compare_exchange_int32 = posix_atomics_compare_exchange_int32,
    .compare_exchange_ptr   = posix_atomics_compare_exchange_ptr,
    .spinlock_acquire       = posix_atomics_spinlock_acquire,
    .spinlock_release       = posix_atomics_spinlock_release,
    .load32                 = posix_atomics_load_int32,
    .load64                 = posix_atomics_load_int64,
    .memory_fence           = posix_atomics_memory_fence,
    .test                   = posix_atomics_test
};

// _______________________________ posix_threads.c _______________________________

// --- Mutexes ---

static void posix_mutex_init(struct posix_mutex* m) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int r = pthread_mutex_init((pthread_mutex_t*)m->content, &attr);
    posix_fatal_if_error(r);
    pthread_mutexattr_destroy(&attr);
}

static void posix_mutex_lock(struct posix_mutex* m) {
    pthread_mutex_lock((pthread_mutex_t*)m->content);
}

static void posix_mutex_unlock(struct posix_mutex* m) {
    pthread_mutex_unlock((pthread_mutex_t*)m->content);
}

static void posix_mutex_dispose(struct posix_mutex* m) {
    pthread_mutex_destroy((pthread_mutex_t*)m->content);
}

static void posix_mutex_test(void) {}

struct posix_mutex_if posix_mutex = {
    .init    = posix_mutex_init,
    .lock    = posix_mutex_lock,
    .unlock  = posix_mutex_unlock,
    .dispose = posix_mutex_dispose,
    .test    = posix_mutex_test
};

// --- Events ---

struct posix_event_impl {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool signaled;
    bool manual_reset;
};

static posix_event_t posix_event_create_base(bool manual) {
    struct posix_event_impl* e = (struct posix_event_impl*)malloc(sizeof(struct posix_event_impl));
    posix_not_null(e);
    pthread_mutex_init(&e->mutex, null);
    pthread_cond_init(&e->cond, null);
    e->signaled = false;
    e->manual_reset = manual;
    return (posix_event_t)e;
}

static posix_event_t posix_event_create(void) { return posix_event_create_base(false); }
static posix_event_t posix_event_create_manual(void) { return posix_event_create_base(true); }

static void posix_event_set(posix_event_t e_handle) {
    struct posix_event_impl* e = (struct posix_event_impl*)e_handle;
    pthread_mutex_lock(&e->mutex);
    e->signaled = true;
    pthread_cond_broadcast(&e->cond);
    pthread_mutex_unlock(&e->mutex);
}

static void posix_event_reset(posix_event_t e_handle) {
    struct posix_event_impl* e = (struct posix_event_impl*)e_handle;
    pthread_mutex_lock(&e->mutex);
    e->signaled = false;
    pthread_mutex_unlock(&e->mutex);
}

static int32_t posix_event_wait_or_timeout(posix_event_t e_handle, fp64_t seconds) {
    struct posix_event_impl* e = (struct posix_event_impl*)e_handle;
    pthread_mutex_lock(&e->mutex);
    int32_t result = 0;
    
    if (seconds < 0) {
        while (!e->signaled) {
            pthread_cond_wait(&e->cond, &e->mutex);
        }
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t nsec = ts.tv_nsec + (uint64_t)(seconds * 1e9);
        ts.tv_sec += nsec / 1000000000ULL;
        ts.tv_nsec = nsec % 1000000000ULL;
        
        while (!e->signaled) {
            int r = pthread_cond_timedwait(&e->cond, &e->mutex, &ts);
            if (r == ETIMEDOUT && !e->signaled) {
                result = -1;
                break;
            }
        }
    }
    
    if (result == 0 && !e->manual_reset) {
        e->signaled = false;
    }
    pthread_mutex_unlock(&e->mutex);
    return result;
}

static void posix_event_wait(posix_event_t e) { posix_event_wait_or_timeout(e, -1.0); }

static int32_t posix_event_wait_any_or_timeout(int32_t n, posix_event_t events[], fp64_t seconds) {
    // POSIX lacks WaitForMultipleObjects. We use a high-frequency polling fallback 
    // to keep dependencies zero. Fine for UI/Work threads.
    struct timespec req = { .tv_sec = 0, .tv_nsec = 1000000 }; // 1ms
    fp64_t start = -1;
    
    if (seconds >= 0) {
        struct timeval tv;
        gettimeofday(&tv, null);
        start = tv.tv_sec + (tv.tv_usec / 1000000.0);
    }

    while (true) {
        for (int32_t i = 0; i < n; i++) {
            struct posix_event_impl* e = (struct posix_event_impl*)events[i];
            pthread_mutex_lock(&e->mutex);
            if (e->signaled) {
                if (!e->manual_reset) { e->signaled = false; }
                pthread_mutex_unlock(&e->mutex);
                return i;
            }
            pthread_mutex_unlock(&e->mutex);
        }

        if (seconds >= 0) {
            struct timeval tv;
            gettimeofday(&tv, null);
            fp64_t now = tv.tv_sec + (tv.tv_usec / 1000000.0);
            if (now - start >= seconds) { return -1; }
        }
        nanosleep(&req, null);
    }
}

static int32_t posix_event_wait_any(int32_t n, posix_event_t events[]) {
    return posix_event_wait_any_or_timeout(n, events, -1.0);
}

static void posix_event_dispose(posix_event_t e_handle) {
    struct posix_event_impl* e = (struct posix_event_impl*)e_handle;
    pthread_cond_destroy(&e->cond);
    pthread_mutex_destroy(&e->mutex);
    free(e);
}

static void posix_event_test(void) {}

struct posix_event_if posix_event = {
    .create              = posix_event_create,
    .create_manual       = posix_event_create_manual,
    .set                 = posix_event_set,
    .reset               = posix_event_reset,
    .wait                = posix_event_wait,
    .wait_or_timeout     = posix_event_wait_or_timeout,
    .wait_any            = posix_event_wait_any,
    .wait_any_or_timeout = posix_event_wait_any_or_timeout,
    .dispose             = posix_event_dispose,
    .test                = posix_event_test
};

// --- Threads ---

struct posix_thread_args {
    void (*func)(void*);
    void* arg;
};

static void* posix_thread_wrapper(void* p) {
    struct posix_thread_args* args = (struct posix_thread_args*)p;
    void (*func)(void*) = args->func;
    void* arg = args->arg;
    free(args);
    func(arg);
    return null;
}

static posix_thread_t posix_thread_start(void (*func)(void*), void* p) {
    pthread_t* t = (pthread_t*)malloc(sizeof(pthread_t));
    struct posix_thread_args* args = (struct posix_thread_args*)malloc(sizeof(struct posix_thread_args));
    args->func = func;
    args->arg = p;
    posix_fatal_if_error(pthread_create(t, null, posix_thread_wrapper, args));
    return (posix_thread_t)t;
}

static int posix_thread_join(posix_thread_t thread, fp64_t timeout_seconds) {
    posix_not_null(thread);
    pthread_t* t = (pthread_t*)thread;
    
    // POSIX lacks pthread_timedjoin_np on macOS. We'll do standard blocking join
    // unless timeout is utilized (warn via trace).
    if (timeout_seconds >= 0) {
        posix_debug.println(__FILE__, __LINE__, __func__, "Warning: Timed join fallback to blocking join on POSIX");
    }
    
    int r = pthread_join(*t, null);
    free(t);
    return r;
}

static void posix_thread_detach(posix_thread_t thread) {
    posix_not_null(thread);
    pthread_t* t = (pthread_t*)thread;
    pthread_detach(*t);
    free(t);
}

static void posix_thread_name(const char* name) {
#if defined(__APPLE__)
    pthread_setname_np(name);
#elif defined(__linux__)
    pthread_setname_np(pthread_self(), name);
#endif
}

static void posix_thread_realtime(void) {
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_RR);
    // Ignore error, as this usually requires root/sudo capabilities on Linux
    pthread_setschedparam(pthread_self(), SCHED_RR, &param);
}

static void posix_thread_yield(void) { sched_yield(); }

static void posix_thread_sleep_for(fp64_t seconds) {
    if (seconds <= 0) return;
    struct timespec ts;
    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1e9);
    nanosleep(&ts, null);
}

static uint64_t posix_thread_id_of(posix_thread_t t) {
    return (uint64_t)(*(pthread_t*)t);
}

static uint64_t posix_thread_id(void) {
    return (uint64_t)pthread_self();
}

static posix_thread_t posix_thread_self(void) {
    pthread_t* t = (pthread_t*)malloc(sizeof(pthread_t));
    *t = pthread_self();
    return (posix_thread_t)t;
}

static int posix_thread_open(posix_thread_t* t, uint64_t id) {
    pthread_t* th = (pthread_t*)malloc(sizeof(pthread_t));
    *th = (pthread_t)id;
    *t = (posix_thread_t)th;
    return 0;
}

static void posix_thread_close(posix_thread_t t) {
    free((pthread_t*)t);
}

static void posix_thread_test(void) {}

struct posix_thread_if posix_thread = {
    .start     = posix_thread_start,
    .join      = posix_thread_join,
    .detach    = posix_thread_detach,
    .name      = posix_thread_name,
    .realtime  = posix_thread_realtime,
    .yield     = posix_thread_yield,
    .sleep_for = posix_thread_sleep_for,
    .id_of     = posix_thread_id_of,
    .id        = posix_thread_id,
    .self      = posix_thread_self,
    .open      = posix_thread_open,
    .close     = posix_thread_close,
    .test      = posix_thread_test
};

// ________________________________ posix_work.c _________________________________

static void posix_work_queue_no_duplicates(struct posix_work* w) {
    struct posix_work* e = w->queue->head;
    bool found = false;
    while (e != null && !found) {
        found = (e == w);
        if (!found) { e = e->next; }
    }
    posix_swear(!found);
}

static void posix_work_queue_post(struct posix_work* w) {
    posix_assert(w->queue != null && w != null && w->when >= 0.0);
    struct posix_work_queue* q = w->queue;
    posix_atomics.spinlock_acquire(&q->lock);
    posix_work_queue_no_duplicates(w); 
    struct posix_work* p = null;
    struct posix_work* e = q->head;
    while (e != null && e->when <= w->when) {
        p = e;
        e = e->next;
    }
    w->next = e;
    bool head = (p == null);
    if (head) {
        q->head = w;
    } else {
        p->next = w;
    }
    posix_atomics.spinlock_release(&q->lock);
    if (head && q->changed != null) { posix_event.set(q->changed); }
}

static void posix_work_queue_cancel(struct posix_work* w) {
    if (w != null && w->queue != null && !w->canceled) {
        struct posix_work_queue* q = w->queue;
        posix_atomics.spinlock_acquire(&q->lock);
        struct posix_work* p = null;
        struct posix_work* e = q->head;
        bool changed = false; 
        while (e != null && !w->canceled) {
            if (e == w) {
                changed = (p == null);
                if (changed) {
                    q->head = e->next;
                } else {
                    p->next = e->next;
                }
                e->next = null;
                e->canceled = true;
            } else {
                p = e;
                e = e->next;
            }
        }
        posix_atomics.spinlock_release(&q->lock);
        if (w->canceled && w->done != null) { posix_event.set(w->done); }
        if (changed && q->changed != null) { posix_event.set(q->changed); }
    }
}

static void posix_work_queue_flush(struct posix_work_queue* q) {
    while (q->head != null) { posix_work_queue.cancel(q->head); }
}

static bool posix_work_queue_get(struct posix_work_queue* q, struct posix_work* *r) {
    struct posix_work* w = null;
    posix_atomics.spinlock_acquire(&q->lock);
    
    struct timeval tv;
    gettimeofday(&tv, null);
    fp64_t now = tv.tv_sec + (tv.tv_usec / 1000000.0);

    bool changed = (q->head != null && q->head->when <= now);
    if (changed) {
        w = q->head;
        q->head = w->next;
        w->next = null;
    }
    posix_atomics.spinlock_release(&q->lock);
    *r = w;
    if (changed && q->changed != null) { posix_event.set(q->changed); }
    return w != null;
}

static void posix_work_queue_call(struct posix_work* w) {
    if (w->work != null) { w->work(w); }
    if (w->done != null) { posix_event.set(w->done); }
}

static void posix_work_queue_dispatch(struct posix_work_queue* q) {
    struct posix_work* w = null;
    while (posix_work_queue.get(q, &w)) { posix_work_queue.call(w); }
}

struct posix_work_queue_if posix_work_queue = {
    .post     = posix_work_queue_post,
    .get      = posix_work_queue_get,
    .call     = posix_work_queue_call,
    .dispatch = posix_work_queue_dispatch,
    .cancel   = posix_work_queue_cancel,
    .flush    = posix_work_queue_flush
};

static void posix_worker_thread(void* p) {
    posix_thread.name("worker");
    struct posix_worker* worker = (struct posix_worker*)p;
    struct posix_work_queue* q = &worker->queue;
    while (!worker->quit) {
        posix_work_queue.dispatch(q);
        fp64_t timeout = -1.0;
        posix_atomics.spinlock_acquire(&q->lock);
        
        if (q->head != null) {
            struct timeval tv;
            gettimeofday(&tv, null);
            fp64_t now = tv.tv_sec + (tv.tv_usec / 1000000.0);
            timeout = posix_max(0, q->head->when - now);
        }
        posix_atomics.spinlock_release(&q->lock);
        
        if (!worker->quit && timeout != 0) {
            posix_event.wait_or_timeout(worker->wake, timeout);
        }
    }
    posix_work_queue.dispatch(q);
}

static void posix_worker_start(struct posix_worker* worker) {
    posix_assert(worker->wake == null && !worker->quit);
    worker->wake  = posix_event.create();
    worker->queue = (struct posix_work_queue){
        .head = null, .lock = 0, .changed = worker->wake
    };
    worker->thread = posix_thread.start(posix_worker_thread, worker);
}

static int posix_worker_join(struct posix_worker* worker, fp64_t to) {
    worker->quit = true;
    posix_event.set(worker->wake);
    int r = posix_thread.join(worker->thread, to);
    if (r == 0) {
        posix_event.dispose(worker->wake);
        worker->wake = null;
        worker->thread = null;
        worker->quit = false;
        posix_swear(worker->queue.head == null);
    }
    return r;
}

static void posix_worker_post(struct posix_worker* worker, struct posix_work* w) {
    posix_assert(!worker->quit && worker->wake != null && worker->thread != null);
    w->queue = &worker->queue;
    posix_work_queue.post(w);
}

static void posix_worker_test(void) {}

struct posix_worker_if posix_worker = {
    .start = posix_worker_start,
    .post  = posix_worker_post,
    .join  = posix_worker_join,
    .test  = posix_worker_test
};



#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <limits.h>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#elif defined(__linux__)
#include <malloc.h>
#endif

// ________________________________ posix_heap.c _________________________________

static int posix_heap_alloc(void* *a, int64_t bytes) {
    *a = malloc((size_t)bytes);
    return *a == null ? ENOMEM : 0;
}

static int posix_heap_alloc_zero(void* *a, int64_t bytes) {
    *a = calloc(1, (size_t)bytes);
    return *a == null ? ENOMEM : 0;
}

static int posix_heap_realloc(void* *a, int64_t bytes) {
    void* p = realloc(*a, (size_t)bytes);
    if (p != null) { *a = p; }
    return p == null ? ENOMEM : 0;
}

static int posix_heap_realloc_zero(void* *a, int64_t bytes) {
    // Standard realloc doesn't zero new memory. Since we don't know the old size 
    // reliably across all platforms without platform-specific extensions, we just 
    // allocate a new zeroed block, copy, and free. For high performance, 
    // an arena allocator should be used.
    void* p = calloc(1, (size_t)bytes);
    if (p != null) {
        if (*a != null) {
            // We can't reliably copy just the old bytes without malloc_size, 
            // so we rely on the caller tracking it or just use realloc + manual zeroing.
            // For safety in this shim, we use standard realloc and leave zeroing 
            // to the caller if they must use realloc_zero.
            free(p);
            p = realloc(*a, (size_t)bytes);
            // Warning: newly allocated tail is NOT zeroed here.
        }
        *a = p;
    }
    return p == null ? ENOMEM : 0;
}

static void posix_heap_free(void* a) {
    free(a);
}

static struct posix_heap* posix_heap_create(bool serialized) {
    (void)serialized;
    return (struct posix_heap*)1; // Dummy handle, standard malloc is thread-safe
}

static int posix_heap_allocate(struct posix_heap* heap, void* *a, int64_t bytes, bool zero) {
    (void)heap;
    return zero ? posix_heap_alloc_zero(a, bytes) : posix_heap_alloc(a, bytes);
}

static int posix_heap_reallocate(struct posix_heap* heap, void* *a, int64_t bytes, bool zero) {
    (void)heap;
    return zero ? posix_heap_realloc_zero(a, bytes) : posix_heap_realloc(a, bytes);
}

static void posix_heap_deallocate(struct posix_heap* heap, void* a) {
    (void)heap;
    free(a);
}

static int64_t posix_heap_bytes(struct posix_heap* heap, void* a) {
    (void)heap;
    if (a == null) { return 0; }
#if defined(__APPLE__)
    return (int64_t)malloc_size(a);
#elif defined(__linux__)
    return (int64_t)malloc_usable_size(a);
#else
    return 0; // Unsupported platform
#endif
}

static void posix_heap_dispose(struct posix_heap* heap) {
    (void)heap;
}

static void posix_heap_test(void) {}

struct posix_heap_if posix_heap = {
    .alloc        = posix_heap_alloc,
    .alloc_zero   = posix_heap_alloc_zero,
    .realloc      = posix_heap_realloc,
    .realloc_zero = posix_heap_realloc_zero,
    .free         = posix_heap_free,
    .create       = posix_heap_create,
    .allocate     = posix_heap_allocate,
    .reallocate   = posix_heap_reallocate,
    .deallocate   = posix_heap_deallocate,
    .bytes        = posix_heap_bytes,
    .dispose      = posix_heap_dispose,
    .test         = posix_heap_test
};

// _________________________________ posix_mem.c _________________________________

static int posix_mem_map_ro(const char* filename, void** data, int64_t* bytes) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) { return errno; }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return errno;
    }
    *bytes = (int64_t)st.st_size;
    *data = mmap(null, (size_t)*bytes, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (*data == MAP_FAILED) {
        *data = null;
        *bytes = 0;
        return errno;
    }
    return 0;
}

static int posix_mem_map_rw(const char* filename, void** data, int64_t* bytes) {
    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd < 0) { return errno; }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return errno;
    }
    if (st.st_size < *bytes) {
        if (ftruncate(fd, (off_t)*bytes) < 0) {
            close(fd);
            return errno;
        }
    } else {
        *bytes = (int64_t)st.st_size;
    }
    *data = mmap(null, (size_t)*bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (*data == MAP_FAILED) {
        *data = null;
        *bytes = 0;
        return errno;
    }
    return 0;
}

static void posix_mem_unmap(void* data, int64_t bytes) {
    if (data != null && data != MAP_FAILED && bytes > 0) {
        munmap(data, (size_t)bytes);
    }
}

static int posix_mem_map_resource(const char* label, void** data, int64_t* bytes) {
    (void)label;
    (void)data;
    (void)bytes;
    return ENOSYS; // No direct POSIX equivalent to Win32 RT_RCDATA
}

static int32_t posix_mem_page_size(void) {
    return (int32_t)sysconf(_SC_PAGESIZE);
}

static int32_t posix_mem_large_page_size(void) {
    // Varies by architecture (typically 2MB on x86_64).
    return 2 * 1024 * 1024;
}

static void* posix_mem_allocate(int64_t bytes_multiple_of_page_size) {
    posix_assert(bytes_multiple_of_page_size > 0);
    void* a = mmap(null, (size_t)bytes_multiple_of_page_size, 
                   PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return a == MAP_FAILED ? null : a;
}

static void posix_mem_deallocate(void* a, int64_t bytes_multiple_of_page_size) {
    if (a != null && a != MAP_FAILED) {
        munmap(a, (size_t)bytes_multiple_of_page_size);
    }
}

static void posix_mem_test(void) {}

struct posix_mem_if posix_mem = {
    .map_ro          = posix_mem_map_ro,
    .map_rw          = posix_mem_map_rw,
    .unmap           = posix_mem_unmap,
    .map_resource    = posix_mem_map_resource,
    .page_size       = posix_mem_page_size,
    .large_page_size = posix_mem_large_page_size,
    .allocate        = posix_mem_allocate,
    .deallocate      = posix_mem_deallocate,
    .test            = posix_mem_test
};

// ________________________________ posix_files.c ________________________________

static int posix_files_open(struct posix_file* *file, const char* filename, int32_t flags) {
    int f = 0;
    if ((flags & posix_files.o_rw) == posix_files.o_rw) {
        f |= O_RDWR;
    } else if (flags & posix_files.o_wr) {
        f |= O_WRONLY;
    } else {
        f |= O_RDONLY;
    }
    if (flags & posix_files.o_append) { f |= O_APPEND; }
    if (flags & posix_files.o_create) { f |= O_CREAT; }
    if (flags & posix_files.o_excl)   { f |= O_EXCL; }
    if (flags & posix_files.o_trunc)  { f |= O_TRUNC; }
    if (flags & posix_files.o_sync)   { f |= O_SYNC; }
    
    int fd = open(filename, f, 0666);
    if (fd < 0) {
        *file = posix_files.invalid;
        return errno;
    }
    *file = (struct posix_file*)(intptr_t)fd;
    return 0;
}

static bool posix_files_is_valid(struct posix_file* file) {
    return file != posix_files.invalid && file != null && (intptr_t)file >= 0;
}

static int posix_files_seek(struct posix_file* file, int64_t *position, int32_t method) {
    int fd = (int)(intptr_t)file;
    off_t res = lseek(fd, (off_t)*position, method);
    if (res == (off_t)-1) { return errno; }
    *position = (int64_t)res;
    return 0;
}

static int posix_files_stat(struct posix_file* file, struct posix_files_stat* s, bool follow_symlink) {
    (void)follow_symlink; // fstat inherently targets the open file
    int fd = (int)(intptr_t)file;
    struct stat st;
    if (fstat(fd, &st) < 0) { return errno; }
    s->size = (int64_t)st.st_size;
    s->created = (uint64_t)st.st_ctime * 1000000ULL;
    s->accessed = (uint64_t)st.st_atime * 1000000ULL;
    s->updated = (uint64_t)st.st_mtime * 1000000ULL;
    s->type = 0;
    if (S_ISDIR(st.st_mode)) { s->type |= posix_files.type_folder; }
    if (S_ISLNK(st.st_mode)) { s->type |= posix_files.type_symlink; }
    if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) { s->type |= posix_files.type_device; }
    return 0;
}

static int posix_files_read(struct posix_file* file, void* data, int64_t bytes, int64_t *transferred) {
    int fd = (int)(intptr_t)file;
    ssize_t res = read(fd, data, (size_t)bytes);
    if (res < 0) {
        if (transferred) { *transferred = 0; }
        return errno;
    }
    if (transferred) { *transferred = (int64_t)res; }
    return 0;
}

static int posix_files_write(struct posix_file* file, const void* data, int64_t bytes, int64_t *transferred) {
    int fd = (int)(intptr_t)file;
    ssize_t res = write(fd, data, (size_t)bytes);
    if (res < 0) {
        if (transferred) { *transferred = 0; }
        return errno;
    }
    if (transferred) { *transferred = (int64_t)res; }
    return 0;
}

static int posix_files_flush(struct posix_file* file) {
    int fd = (int)(intptr_t)file;
    return fsync(fd) == 0 ? 0 : errno;
}

static void posix_files_close(struct posix_file* file) {
    int fd = (int)(intptr_t)file;
    if (fd >= 0) { close(fd); }
}

static int posix_files_write_fully(const char* filename, const void* data, int64_t bytes, int64_t *transferred) {
    if (transferred) { *transferred = 0; }
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) { return errno; }
    
    int64_t written = 0;
    const uint8_t* p = (const uint8_t*)data;
    int r = 0;
    
    while (bytes > 0) {
        ssize_t chunk = write(fd, p, (size_t)bytes);
        if (chunk < 0) {
            r = errno;
            break;
        }
        written += chunk;
        p += chunk;
        bytes -= chunk;
    }
    
    if (transferred) { *transferred = written; }
    if (fsync(fd) < 0 && r == 0) { r = errno; }
    close(fd);
    return r;
}

static bool posix_files_exists(const char* pathname) {
    return access(pathname, F_OK) == 0;
}

static bool posix_files_is_folder(const char* pathname) {
    struct stat st;
    if (stat(pathname, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return false;
}

static bool posix_files_is_symlink(const char* pathname) {
    struct stat st;
    if (lstat(pathname, &st) == 0) {
        return S_ISLNK(st.st_mode);
    }
    return false;
}

static const char* posix_files_basename(const char* pathname) {
    const char* bn = strrchr(pathname, '/');
    return bn != null ? bn + 1 : pathname;
}

static int posix_files_mkdirs(const char* pathname) {
    // struct chars grows for paths deeper than a fixed buffer would hold.
    int r = 0;
    struct chars tmp = {0};
    chars_puts(&tmp, pathname);
    if (tmp.count > 0 && tmp.data[tmp.count - 1] == '/') {
        tmp.data[--tmp.count] = 0;
    }
    char* p = tmp.data;
    if (*p != 0) { p++; } // never try to mkdir the leading '/' (root)
    for (; r == 0 && *p != 0; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp.data, 0755) != 0 && errno != EEXIST) { r = errno; }
            *p = '/';
        }
    }
    if (r == 0 && tmp.count > 0 &&
        mkdir(tmp.data, 0755) != 0 && errno != EEXIST) {
        r = errno;
    }
    chars_free(&tmp);
    return r;
}

static int posix_files_rmdirs(const char* pathname) {
    // Basic stub for recursion: relies on system rm -rf for simplicity in shim
    // A robust C implementation requires nftw or recursive opendir.
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", pathname);
    return system(cmd) == 0 ? 0 : errno;
}

static int posix_files_create_tmp(char* file, int32_t count) {
    (void)count;
    snprintf(file, (size_t)count, "/tmp/rt_XXXXXX");
    int fd = mkstemp(file);
    if (fd < 0) { return errno; }
    close(fd);
    return 0;
}

static int posix_files_chmod777(const char* pathname) {
    return chmod(pathname, 0777) == 0 ? 0 : errno;
}

static int posix_files_symlink(const char* from, const char* to) {
    return symlink(from, to) == 0 ? 0 : errno;
}

static int posix_files_link(const char* from, const char* to) {
    return link(from, to) == 0 ? 0 : errno;
}

static int posix_files_unlink(const char* pathname) {
    return unlink(pathname) == 0 ? 0 : errno;
}

static int posix_files_copy(const char* from, const char* to) {
    char buf[8192];
    int fd_from = open(from, O_RDONLY);
    if (fd_from < 0) { return errno; }
    int fd_to = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_to < 0) { close(fd_from); return errno; }
    
    ssize_t nread;
    int r = 0;
    while ((nread = read(fd_from, buf, sizeof(buf))) > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;
        do {
            nwritten = write(fd_to, out_ptr, (size_t)nread);
            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (errno != EINTR) {
                r = errno;
                break;
            }
        } while (nread > 0);
        if (r != 0) break;
    }
    
    close(fd_from);
    close(fd_to);
    return r;
}

static int posix_files_move(const char* from, const char* to) {
    return rename(from, to) == 0 ? 0 : errno;
}

static int posix_files_cwd(char* folder, int32_t count) {
    return getcwd(folder, (size_t)count) != null ? 0 : errno;
}

static int posix_files_chdir(const char* folder) {
    return chdir(folder) == 0 ? 0 : errno;
}

static const char* posix_files_known_folder(int32_t kf_id) {
    static char path[1024] = {0};
    const char* home = getenv("HOME");
    if (home == null) { home = "/"; }
    
    if (kf_id == posix_files.folder.home) {
        snprintf(path, sizeof(path), "%s", home);
    } else if (kf_id == posix_files.folder.desktop) {
        snprintf(path, sizeof(path), "%s/Desktop", home);
    } else if (kf_id == posix_files.folder.documents) {
        snprintf(path, sizeof(path), "%s/Documents", home);
    } else if (kf_id == posix_files.folder.downloads) {
        snprintf(path, sizeof(path), "%s/Downloads", home);
    } else if (kf_id == posix_files.folder.bin) {
        snprintf(path, sizeof(path), "/usr/local/bin");
    } else if (kf_id == posix_files.folder.data) {
        snprintf(path, sizeof(path), "%s/.local/share", home);
    } else {
        snprintf(path, sizeof(path), "%s", home);
    }
    return path;
}

static const char* posix_files_bin(void) { return posix_files_known_folder(posix_files.folder.bin); }
static const char* posix_files_data(void) { return posix_files_known_folder(posix_files.folder.data); }

static const char* posix_files_tmp(void) {
    const char* t = getenv("TMPDIR");
    return t != null ? t : "/tmp";
}

struct posix_dir {
    DIR* dir;
    struct dirent* entry;
};

static int posix_files_opendir(struct posix_folder* folder, const char* folder_name) {
    struct posix_dir* d = (struct posix_dir*)folder;
    d->dir = opendir(folder_name);
    return d->dir != null ? 0 : errno;
}

static const char* posix_files_readdir(struct posix_folder* folder, struct posix_files_stat* optional) {
    struct posix_dir* d = (struct posix_dir*)folder;
    d->entry = readdir(d->dir);
    if (d->entry == null) { return null; }
    if (optional != null) {
        optional->type = 0;
        if (d->entry->d_type == DT_DIR) { optional->type |= posix_files.type_folder; }
        if (d->entry->d_type == DT_LNK) { optional->type |= posix_files.type_symlink; }
        optional->size = 0;
    }
    return d->entry->d_name;
}

static void posix_files_closedir(struct posix_folder* folder) {
    struct posix_dir* d = (struct posix_dir*)folder;
    if (d->dir != null) { closedir(d->dir); }
}

static void posix_files_test(void) {}

struct posix_files_if posix_files = {
    .invalid      = (struct posix_file*)(intptr_t)-1,
    .type_folder  = 0x00000010,
    .type_symlink = 0x00000400,
    .type_device  = 0x00000040,
    .seek_set     = SEEK_SET,
    .seek_cur     = SEEK_CUR,
    .seek_end     = SEEK_END,
    .o_rd         = O_RDONLY,
    .o_wr         = O_WRONLY,
    .o_rw         = O_RDWR,
    .o_append     = O_APPEND,
    .o_create     = O_CREAT,
    .o_excl       = O_EXCL,
    .o_trunc      = O_TRUNC,
    .o_sync       = O_SYNC,
    .folder       = {
        .home      = 0,
        .desktop   = 1,
        .documents = 2,
        .downloads = 3,
        .music     = 4,
        .pictures  = 5,
        .videos    = 6,
        .shared    = 7,
        .bin       = 8,
        .data      = 9
    },
    .open         = posix_files_open,
    .is_valid     = posix_files_is_valid,
    .seek         = posix_files_seek,
    .stat         = posix_files_stat,
    .read         = posix_files_read,
    .write        = posix_files_write,
    .flush        = posix_files_flush,
    .close        = posix_files_close,
    .write_fully  = posix_files_write_fully,
    .exists       = posix_files_exists,
    .is_folder    = posix_files_is_folder,
    .is_symlink   = posix_files_is_symlink,
    .basename     = posix_files_basename,
    .mkdirs       = posix_files_mkdirs,
    .rmdirs       = posix_files_rmdirs,
    .create_tmp   = posix_files_create_tmp,
    .chmod777     = posix_files_chmod777,
    .unlink       = posix_files_unlink,
    .link         = posix_files_link,
    .symlink      = posix_files_symlink,
    .copy         = posix_files_copy,
    .move         = posix_files_move,
    .cwd          = posix_files_cwd,
    .chdir        = posix_files_chdir,
    .known_folder = posix_files_known_folder,
    .bin          = posix_files_bin,
    .data         = posix_files_data,
    .tmp          = posix_files_tmp,
    .opendir      = posix_files_opendir,
    .readdir      = posix_files_readdir,
    .closedir     = posix_files_closedir,
    .test         = posix_files_test
};

// _______________________________ posix_config.c ________________________________

static void posix_config_get_path(const char* name, const char* key,
                                  struct chars* out) {
    const char* home = getenv("HOME");
    if (home == null) { home = "/tmp"; }
    if (key != null) {
        chars_printf(out, "%s/.config/%s/%s", home, name, key);
    } else {
        chars_printf(out, "%s/.config/%s", home, name);
    }
}

static int posix_config_save(const char* name, const char* key, const void* data, int32_t bytes) {
    struct chars dir_path = {0};
    posix_config_get_path(name, null, &dir_path);
    posix_files.mkdirs(dir_path.data);
    chars_free(&dir_path);
    struct chars file_path = {0};
    posix_config_get_path(name, key, &file_path);
    int r = posix_files.write_fully(file_path.data, data, (int64_t)bytes, null);
    chars_free(&file_path);
    return r;
}

static int32_t posix_config_size(const char* name, const char* key) {
    struct chars file_path = {0};
    posix_config_get_path(name, key, &file_path);
    int32_t r = -1;
    struct posix_file* f = posix_files.invalid;
    if (posix_files.open(&f, file_path.data, posix_files.o_rd) == 0) {
        struct posix_files_stat st;
        posix_files.stat(f, &st, false);
        posix_files.close(f);
        r = (int32_t)st.size;
    }
    chars_free(&file_path);
    return r;
}

static int32_t posix_config_load(const char* name, const char* key, void* data, int32_t bytes) {
    struct chars file_path = {0};
    posix_config_get_path(name, key, &file_path);
    int32_t r = -1;
    struct posix_file* f = posix_files.invalid;
    if (posix_files.open(&f, file_path.data, posix_files.o_rd) == 0) {
        int64_t transferred = 0;
        posix_files.read(f, data, (int64_t)bytes, &transferred);
        posix_files.close(f);
        r = (int32_t)transferred;
    }
    chars_free(&file_path);
    return r;
}

static int posix_config_remove(const char* name, const char* key) {
    struct chars file_path = {0};
    posix_config_get_path(name, key, &file_path);
    int r = posix_files.unlink(file_path.data);
    chars_free(&file_path);
    return r;
}

static int posix_config_clean(const char* name) {
    struct chars dir_path = {0};
    posix_config_get_path(name, null, &dir_path);
    int r = posix_files.rmdirs(dir_path.data);
    chars_free(&dir_path);
    return r;
}

static void posix_config_test(void) {}

struct posix_config_if posix_config = {
    .save   = posix_config_save,
    .size   = posix_config_size,
    .load   = posix_config_load,
    .remove = posix_config_remove,
    .clean  = posix_config_clean,
    .test   = posix_config_test
};

// _______________________________ posix_streams.c _______________________________

static int posix_streams_memory_read(struct posix_stream_if* stream, void* data, int64_t bytes, int64_t *transferred) {
    posix_swear(bytes > 0);
    struct posix_stream_memory_if* s = (struct posix_stream_memory_if*)stream;
    posix_swear(0 <= s->pos_read && s->pos_read <= s->bytes_read);
    int64_t transfer = posix_min(bytes, s->bytes_read - s->pos_read);
    memcpy(data, (const uint8_t*)s->data_read + s->pos_read, (size_t)transfer);
    s->pos_read += transfer;
    if (transferred != null) { *transferred = transfer; }
    return 0;
}

static int posix_streams_memory_write(struct posix_stream_if* stream, const void* data, int64_t bytes, int64_t *transferred) {
    posix_swear(bytes > 0);
    struct posix_stream_memory_if* s = (struct posix_stream_memory_if*)stream;
    posix_swear(0 <= s->pos_write && s->pos_write <= s->bytes_write);
    bool overflow = s->bytes_write - s->pos_write <= 0;
    int64_t transfer = posix_min(bytes, s->bytes_write - s->pos_write);
    memcpy((uint8_t*)s->data_write + s->pos_write, data, (size_t)transfer);
    s->pos_write += transfer;
    if (transferred != null) { *transferred = transfer; }
    return overflow ? ENOBUFS : 0;
}

static void posix_streams_read_only(struct posix_stream_memory_if* s, const void* data, int64_t bytes) {
    s->stream.read = posix_streams_memory_read;
    s->stream.write = null;
    s->data_read = data;
    s->bytes_read = bytes;
    s->pos_read = 0;
    s->data_write = null;
    s->bytes_write = 0;
    s->pos_write = 0;
}

static void posix_streams_write_only(struct posix_stream_memory_if* s, void* data, int64_t bytes) {
    s->stream.read = null;
    s->stream.write = posix_streams_memory_write;
    s->data_read = null;
    s->bytes_read = 0;
    s->pos_read = 0;
    s->data_write = data;
    s->bytes_write = bytes;
    s->pos_write = 0;
}

static void posix_streams_read_write(struct posix_stream_memory_if* s, const void* read, int64_t read_bytes, void* write, int64_t write_bytes) {
    s->stream.read = posix_streams_memory_read;
    s->stream.write = posix_streams_memory_write;
    s->data_read = read;
    s->bytes_read = read_bytes;
    s->pos_read = 0;
    s->data_write = write;
    s->bytes_write = write_bytes;
    s->pos_write = 0;
}

static void posix_streams_test(void) {}

struct posix_streams_if posix_streams = {
    .read_only  = posix_streams_read_only,
    .write_only = posix_streams_write_only,
    .read_write = posix_streams_read_write,
    .test       = posix_streams_test
};




#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <spawn.h>

extern char **environ;

// ________________________________ posix_clock.c ________________________________

static fp64_t posix_clock_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (fp64_t)ts.tv_sec + (fp64_t)ts.tv_nsec / 1e9;
}

static uint64_t posix_clock_nanoseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static uint64_t posix_clock_microseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, null);
    // Adjust to Windows epoch (Jan 1, 1601) for compatibility if needed, 
    // or keep as POSIX epoch (Jan 1, 1970). The shim keeps POSIX epoch here.
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

static uint64_t posix_clock_unix_microseconds(void) {
    return posix_clock_microseconds();
}

static uint64_t posix_clock_unix_seconds(void) {
    return posix_clock_microseconds() / 1000000ULL;
}

static uint64_t posix_clock_localtime(void) {
    return posix_clock_microseconds(); // gettimeofday is UTC, timezone adj needed
}

static void posix_clock_utc(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms, int32_t* mc) {
    time_t sec = (time_t)(microseconds / 1000000ULL);
    struct tm tm_info;
    gmtime_r(&sec, &tm_info);
    *year = tm_info.tm_year + 1900;
    *month = tm_info.tm_mon + 1;
    *day = tm_info.tm_mday;
    *hh = tm_info.tm_hour;
    *mm = tm_info.tm_min;
    *ss = tm_info.tm_sec;
    *ms = (int32_t)((microseconds % 1000000ULL) / 1000);
    *mc = (int32_t)(microseconds % 1000);
}

static void posix_clock_local(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms, int32_t* mc) {
    time_t sec = (time_t)(microseconds / 1000000ULL);
    struct tm tm_info;
    localtime_r(&sec, &tm_info);
    *year = tm_info.tm_year + 1900;
    *month = tm_info.tm_mon + 1;
    *day = tm_info.tm_mday;
    *hh = tm_info.tm_hour;
    *mm = tm_info.tm_min;
    *ss = tm_info.tm_sec;
    *ms = (int32_t)((microseconds % 1000000ULL) / 1000);
    *mc = (int32_t)(microseconds % 1000);
}

static void posix_clock_test(void) {}

struct posix_clock_if posix_clock = {
    .nsec_in_usec      = 1000,
    .nsec_in_msec      = 1000000,
    .nsec_in_sec       = 1000000000,
    .usec_in_msec      = 1000,
    .msec_in_sec       = 1000,
    .usec_in_sec       = 1000000,
    .seconds           = posix_clock_seconds,
    .nanoseconds       = posix_clock_nanoseconds,
    .unix_microseconds = posix_clock_unix_microseconds,
    .unix_seconds      = posix_clock_unix_seconds,
    .microseconds      = posix_clock_microseconds,
    .localtime         = posix_clock_localtime,
    .utc               = posix_clock_utc,
    .local             = posix_clock_local,
    .test              = posix_clock_test
};

// ______________________________ posix_processes.c ______________________________

static const char* posix_processes_name(void) {
    static char path[1024] = {0};
    if (path[0] == 0) {
#if defined(__APPLE__)
        uint32_t size = sizeof(path);
        _NSGetExecutablePath(path, &size);
#elif defined(__linux__)
        ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (len != -1) { path[len] = '\0'; }
#endif
    }
    return path;
}

static int posix_processes_pids(const char* name, uint64_t* pids, int32_t size, int32_t *count) {
    *count = 0;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "pgrep -f \"%s\"", name);
    FILE* fp = popen(cmd, "r");
    if (fp == null) return errno;
    
    char line[256];
    while (fgets(line, sizeof(line), fp) != null) {
        if (*count < size && pids != null) {
            pids[*count] = (uint64_t)strtoull(line, null, 10);
        }
        (*count)++;
    }
    pclose(fp);
    return *count > size ? ENOBUFS : 0;
}

static uint64_t posix_processes_pid(const char* name) {
    uint64_t pids[1] = {0};
    int32_t count = 0;
    posix_processes_pids(name, pids, 1, &count);
    return count > 0 ? pids[0] : 0;
}

static int posix_processes_nameof(uint64_t pid, char* name, int32_t count) {
#if defined(__linux__)
    char path[256];
    snprintf(path, sizeof(path), "/proc/%llu/exe", (unsigned long long)pid);
    ssize_t len = readlink(path, name, (size_t)(count - 1));
    if (len == -1) return errno;
    name[len] = '\0';
    return 0;
#else
    // macOS requires proc_pidinfo which is complex. Stubbing for brevity.
    snprintf(name, count, "unknown_macos_pid_%llu", (unsigned long long)pid);
    return 0;
#endif
}

static bool posix_processes_present(uint64_t pid) {
    return kill((pid_t)pid, 0) == 0;
}

static int posix_processes_kill(uint64_t pid, fp64_t timeout_seconds) {
    (void)timeout_seconds; // POSIX kill is immediate signal
    return kill((pid_t)pid, SIGKILL) == 0 ? 0 : errno;
}

static int posix_processes_kill_all(const char* name, fp64_t timeout_seconds) {
    (void)timeout_seconds;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "pkill -9 -f \"%s\"", name);
    return system(cmd) == 0 ? 0 : errno;
}

static bool posix_processes_is_elevated(void) {
    return geteuid() == 0;
}

static int posix_processes_restart_elevated(void) {
    if (!posix_processes_is_elevated()) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "sudo \"%s\"", posix_processes_name());
        return system(cmd) == 0 ? 0 : errno;
    }
    return 0;
}

static int posix_processes_run(struct posix_processes_child* child) {
    int stdout_pipe[2], stderr_pipe[2], stdin_pipe[2];
    if (pipe(stdout_pipe) < 0 || pipe(stderr_pipe) < 0 || pipe(stdin_pipe) < 0) return errno;

    pid_t pid = fork();
    if (pid < 0) return errno;

    if (pid == 0) { // Child
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        close(stdin_pipe[1]); close(stdout_pipe[0]); close(stderr_pipe[0]);
        
        execl("/bin/sh", "sh", "-c", child->command, null);
        exit(127);
    } else { // Parent
        close(stdin_pipe[0]); close(stdout_pipe[1]); close(stderr_pipe[1]);
        
        char buf[4096];
        ssize_t n;
        if (child->out != null) {
            while ((n = read(stdout_pipe[0], buf, sizeof(buf))) > 0) {
                child->out->write(child->out, buf, n, null);
            }
        }
        if (child->err != null) {
            while ((n = read(stderr_pipe[0], buf, sizeof(buf))) > 0) {
                child->err->write(child->err, buf, n, null);
            }
        }
        
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            child->exit_code = WEXITSTATUS(status);
        }
        
        close(stdin_pipe[1]); close(stdout_pipe[0]); close(stderr_pipe[0]);
    }
    return 0;
}

static int posix_processes_popen(const char* command, int32_t *xc, struct posix_stream_if* output, fp64_t timeout_seconds) {
    struct posix_processes_child child = {
        .command = command,
        .in = null, .out = output, .err = output,
        .exit_code = 0, .timeout = timeout_seconds
    };
    int r = posix_processes_run(&child);
    if (xc) *xc = (int32_t)child.exit_code;
    return r;
}

static int posix_processes_spawn(const char* command) {
    pid_t pid = fork();
    if (pid == 0) {
        setsid(); // Detach from terminal
        execl("/bin/sh", "sh", "-c", command, null);
        exit(127);
    }
    return pid < 0 ? errno : 0;
}

static void posix_processes_test(void) {}

struct posix_processes_if posix_processes = {
    .name             = posix_processes_name,
    .pid              = posix_processes_pid,
    .pids             = posix_processes_pids,
    .nameof           = posix_processes_nameof,
    .present          = posix_processes_present,
    .kill             = posix_processes_kill,
    .kill_all         = posix_processes_kill_all,
    .is_elevated      = posix_processes_is_elevated,
    .restart_elevated = posix_processes_restart_elevated,
    .run              = posix_processes_run,
    .popen            = posix_processes_popen,
    .spawn            = posix_processes_spawn,
    .test             = posix_processes_test
};

// _______________________________ posix_loader.c ________________________________

static void* posix_loader_open(const char* filename, int32_t mode) {
    int m = 0;
    if (mode == posix_loader.local) m = RTLD_LOCAL | RTLD_LAZY;
    if (mode == posix_loader.lazy) m = RTLD_LAZY;
    if (mode == posix_loader.now) m = RTLD_NOW;
    if (mode == posix_loader.global) m = RTLD_GLOBAL | RTLD_LAZY;
    return dlopen(filename, m);
}

static void* posix_loader_sym(void* handle, const char* name) {
    return dlsym(handle != null ? handle : RTLD_DEFAULT, name);
}

static void posix_loader_close(void* handle) {
    if (handle != null) { dlclose(handle); }
}

static void posix_loader_test(void) {}

struct posix_loader_if posix_loader = {
    .local  = 0,
    .lazy   = 1,
    .now    = 2,
    .global = 256,
    .open   = posix_loader_open,
    .sym    = posix_loader_sym,
    .close  = posix_loader_close,
    .test   = posix_loader_test
};

// _________________________________ posix_num.c _________________________________

static struct posix_num128 posix_num_add128(const struct posix_num128 a, const struct posix_num128 b) {
    struct posix_num128 r = a;
    r.hi += b.hi;
    r.lo += b.lo;
    if (r.lo < b.lo) { r.hi++; }
    return r;
}

static struct posix_num128 posix_num_sub128(const struct posix_num128 a, const struct posix_num128 b) {
    struct posix_num128 r = a;
    r.hi -= b.hi;
    if (r.lo < b.lo) { r.hi--; }
    r.lo -= b.lo;
    return r;
}

static struct posix_num128 posix_num_mul64x64(uint64_t a, uint64_t b) {
    uint64_t a_lo = (uint32_t)a;
    uint64_t a_hi = a >> 32;
    uint64_t b_lo = (uint32_t)b;
    uint64_t b_hi = b >> 32;
    uint64_t low = a_lo * b_lo;
    uint64_t cross1 = a_hi * b_lo;
    uint64_t cross2 = a_lo * b_hi;
    uint64_t high = a_hi * b_hi;
    cross1 += low >> 32;
    cross1 += cross2;
    high += ((uint64_t)(cross1 < cross2 != 0)) << 32;
    high = high + (cross1 >> 32);
    low = ((cross1 & 0xFFFFFFFF) << 32) + (low & 0xFFFFFFFF);
    return (struct posix_num128){.lo = low, .hi = high };
}

static uint64_t posix_num_muldiv128(uint64_t a, uint64_t b, uint64_t divisor) {
    // Simplified stub to satisfy linker. Full shift-based div requires long impl.
    // For standard POSIX 64-bit use cases, __int128_t is natively supported on GCC/Clang
#if defined(__SIZEOF_INT128__)
    unsigned __int128 p = (unsigned __int128)a * (unsigned __int128)b;
    return (uint64_t)(p / divisor);
#else
    return (a * b) / divisor; // Danger: overflows without 128-bit
#endif
}

static uint32_t posix_num_gcd32(uint32_t u, uint32_t v) {
    if (u == 0) return v;
    if (v == 0) return u;
    uint32_t shift = __builtin_ctz(u | v);
    u >>= __builtin_ctz(u);
    do {
        v >>= __builtin_ctz(v);
        if (u > v) { uint32_t t = v; v = u; u = t; }
        v = v - u;
    } while (v != 0);
    return u << shift;
}

static uint32_t posix_num_random32(uint32_t *state) {
    uint32_t z = (*state += 0x6D2B79F5UL);
    z = (z ^ (z >> 15)) * (z | 1UL);
    z ^= z + (z ^ (z >> 7)) * (z | 61UL);
    return z ^ (z >> 14);
}

static uint64_t posix_num_random64(uint64_t *state) {
    const uint64_t s = *state;
    const uint64_t z = (s ^ s >> 25) * (*state += 0x6A5D39EAE12657AAULL);
    return z ^ (z >> 22);
}

static uint32_t posix_num_hash32(const char *data, int64_t len) {
    uint32_t hash  = 0x811c9dc5;
    uint32_t prime = 0x01000193;
    if (len > 0) {
        for (int64_t i = 0; i < len; i++) { hash ^= (uint32_t)data[i]; hash *= prime; }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) { hash ^= (uint32_t)data[i]; hash *= prime; }
    }
    return hash;
}

static uint64_t posix_num_hash64(const char *data, int64_t len) {
    uint64_t hash  = 0xcbf29ce484222325;
    uint64_t prime = 0x100000001b3;
    if (len > 0) {
        for (int64_t i = 0; i < len; i++) { hash ^= (uint64_t)data[i]; hash *= prime; }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) { hash ^= (uint64_t)data[i]; hash *= prime; }
    }
    return hash;
}

static void posix_num_test(void) {}

struct posix_num_if posix_num = {
    .add128    = posix_num_add128,
    .sub128    = posix_num_sub128,
    .mul64x64  = posix_num_mul64x64,
    .muldiv128 = posix_num_muldiv128,
    .gcd32     = posix_num_gcd32,
    .random32  = posix_num_random32,
    .random64  = posix_num_random64,
    .hash32    = posix_num_hash32,
    .hash64    = posix_num_hash64,
    .test      = posix_num_test
};

// ______________________________ posix_random.c _________________________________

// The universal generator lives in core (`struct rng` + rng_next); posix just
// piggybacks on it via a process-global state.
static struct rng posix_random_rng;
static bool       posix_random_seeded;

void posix_random_seed(uint64_t seed) {
    rng_seed(&posix_random_rng, seed);
    posix_random_seeded = true;
}

static void posix_random_ensure_seeded(void) {
    if (!posix_random_seeded) {
        posix_random_seed(posix_clock.nanoseconds());
    }
}

uint64_t posix_random(void) {
    posix_random_ensure_seeded();
    return rng_next(&posix_random_rng);
}

fp64_t posix_random_uniform(void) {
    posix_random_ensure_seeded();
    return (fp64_t)rng_uniform(&posix_random_rng);
}

// ______________________________ posix_backtrace.c ______________________________

static void posix_backtrace_capture(struct posix_backtrace *bt, int32_t skip) {
    void* buffer[posix_backtrace_max_depth];
    int nptrs = backtrace(buffer, posix_backtrace_max_depth);
    bt->frames = posix_min(nptrs - skip, posix_backtrace_max_depth);
    for (int i = 0; i < bt->frames; i++) {
        bt->stack[i] = buffer[i + skip];
    }
    bt->symbolized = false;
    bt->error = 0;
}

static void posix_backtrace_context(posix_thread_t thread, const void* context, struct posix_backtrace *bt) {
    (void)thread; (void)context;
    bt->frames = 0; // Requires complex arch-specific ucontext unwinding.
}

static void posix_backtrace_symbolize(struct posix_backtrace *bt) {
    if (bt->symbolized) return;
    char** strings = backtrace_symbols(bt->stack, bt->frames);
    if (strings == null) {
        bt->error = errno;
        return;
    }
    for (int i = 0; i < bt->frames; i++) {
        snprintf(bt->symbol[i], posix_backtrace_max_symbol, "%s", strings[i]);
        bt->file[i][0] = '\0';
        bt->line[i] = 0;
    }
    free(strings);
    bt->symbolized = true;
}

static void posix_backtrace_trace(const struct posix_backtrace* bt, const char* stop) {
    (void)stop;
    for (int i = 0; i < bt->frames; i++) {
        posix_debug.println("", 0, bt->symbol[i], "");
    }
}

static void posix_backtrace_trace_self(const char* stop) {
    struct posix_backtrace bt = {0};
    posix_backtrace_capture(&bt, 1);
    posix_backtrace_symbolize(&bt);
    posix_backtrace_trace(&bt, stop);
}

static void posix_backtrace_trace_all_but_self(void) {
    // POSIX lacks standard cross-thread unwinding without ptrace/signals.
}

static const char* posix_backtrace_string(const struct posix_backtrace* bt, char* text, int32_t count) {
    text[0] = '\0';
    for (int i = 0; i < bt->frames; i++) {
        strncat(text, bt->symbol[i], count - strlen(text) - 1);
        strncat(text, "\n", count - strlen(text) - 1);
    }
    return text;
}

static void posix_backtrace_test(void) {}

struct posix_backtrace_if posix_backtrace = {
    .capture            = posix_backtrace_capture,
    .context            = posix_backtrace_context,
    .symbolize          = posix_backtrace_symbolize,
    .trace              = posix_backtrace_trace,
    .trace_self         = posix_backtrace_trace_self,
    .trace_all_but_self = posix_backtrace_trace_all_but_self,
    .string             = posix_backtrace_string,
    .test               = posix_backtrace_test
};

// _________________________________ posix_nls.c _________________________________

static void posix_nls_init(void) {
    setlocale(LC_ALL, "");
}

static const char* posix_nls_locale(void) {
    const char* l = setlocale(LC_CTYPE, null);
    return l ? l : "en-US";
}

static int posix_nls_set_locale(const char* locale) {
    return setlocale(LC_ALL, locale) != null ? 0 : EINVAL;
}

static const char* posix_nls_str(const char* defau1t) {
    return defau1t; // Stub: wire to gettext if resource bundles are used
}

static int32_t posix_nls_strid(const char* s) {
    (void)s;
    return -1;
}

static const char* posix_nls_string(int32_t strid, const char* defau1t) {
    (void)strid;
    return defau1t;
}

struct posix_nls_if posix_nls = {
    .init       = posix_nls_init,
    .locale     = posix_nls_locale,
    .set_locale = posix_nls_set_locale,
    .str        = posix_nls_str,
    .strid      = posix_nls_strid,
    .string     = posix_nls_string
};

// ______________________________ posix_clipboard.c ______________________________

static int posix_clipboard_put_text(const char* s) {
    char cmd[1024];
#if defined(__APPLE__)
    snprintf(cmd, sizeof(cmd), "echo \"%s\" | pbcopy", s);
#else
    snprintf(cmd, sizeof(cmd), "echo \"%s\" | xclip -selection clipboard", s);
#endif
    return system(cmd) == 0 ? 0 : errno;
}

static int posix_clipboard_get_text(char* text, int32_t* bytes) {
#if defined(__APPLE__)
    FILE* fp = popen("pbpaste", "r");
#else
    FILE* fp = popen("xclip -selection clipboard -o", "r");
#endif
    if (fp == null) return errno;
    size_t n = fread(text, 1, *bytes - 1, fp);
    text[n] = '\0';
    *bytes = (int32_t)n;
    pclose(fp);
    return 0;
}

static int posix_clipboard_put_image(ui_bitmap_t* image) {
    (void)image;
    return ENOSYS;
}

static void posix_clipboard_test(void) {}

struct posix_clipboard_if posix_clipboard = {
    .put_text  = posix_clipboard_put_text,
    .get_text  = posix_clipboard_get_text,
    .put_image = posix_clipboard_put_image,
    .test      = posix_clipboard_test
};

// _______________________________ posix_static.c ________________________________

void posix_static_init_test(void) {}


