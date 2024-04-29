#include "ut/ut.h"
#include "ut/ut_win32.h"

static void* args_memory;

static void args_main(int32_t argc, const char* argv[], const char** env) {
    swear(args.c == 0 && args.v == null && args.env == null);
    swear(args_memory == null);
    args.c = argc;
    args.v = argv;
    args.env = env;
}

static int32_t args_option_index(const char* option) {
    for (int32_t i = 1; i < args.c; i++) {
        if (strcmp(args.v[i], "--") == 0) { break; } // no options after '--'
        if (strcmp(args.v[i], option) == 0) { return i; }
    }
    return -1;
}

static void args_remove_at(int32_t ix) {
    // returns new argc
    assert(0 < args.c);
    assert(0 < ix && ix < args.c); // cannot remove args.v[0]
    for (int32_t i = ix; i < args.c; i++) {
        args.v[i] = args.v[i + 1];
    }
    args.v[args.c - 1] = "";
    args.c--;
}

static bool args_option_bool(const char* option) {
    int32_t ix = args_option_index(option);
    if (ix > 0) { args_remove_at(ix); }
    return ix > 0;
}

static bool args_option_int(const char* option, int64_t *value) {
    int32_t ix = args_option_index(option);
    if (ix > 0 && ix < args.c - 1) {
        const char* s = args.v[ix + 1];
        int32_t base = (strstr(s, "0x") == s || strstr(s, "0X") == s) ? 16 : 10;
        const char* b = s + (base == 10 ? 0 : 2);
        char* e = null;
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
        args_remove_at(ix); // remove option
        args_remove_at(ix); // remove following number
    }
    return ix > 0;
}

static const char* args_option_str(const char* option) {
    int32_t ix = args_option_index(option);
    const char* s = null;
    if (ix > 0 && ix < args.c - 1) {
        s = args.v[ix + 1];
    } else {
        ix = -1;
    }
    if (ix > 0) {
        args_remove_at(ix); // remove option
        args_remove_at(ix); // remove following string
    }
    return ix > 0 ? s : null;
}

// Terminology: "quote" in the code and comments below
// actually refers to "fp64_t quote mark" and used for brevity.

// TODO: posix like systems
// Looks like all shells support quote marks but
// AFAIK MacOS bash and zsh also allow (and prefer) backslash escaped
// space character. Unclear what other escaping shell and posix compliant
// parser should support.
// Lengthy discussion here:
// https://stackoverflow.com/questions/1706551/parse-string-into-argv-argc

// Microsoft specific argument parsing:
// https://web.archive.org/web/20231115181633/http://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments?view=msvc-170
// Alternative: just use CommandLineToArgvW()

typedef struct { const char* s; char* d; const char* e; } args_pair_t;

static args_pair_t args_parse_backslashes(args_pair_t p) {
    enum { quote = '"', backslash = '\\' };
    const char* s = p.s;
    char* d = p.d;
    swear(*s == backslash);
    int32_t bsc = 0; // number of backslashes
    while (*s == backslash) { s++; bsc++; }
    if (*s == quote) {
        while (bsc > 1 && d < p.e) { *d++ = backslash; bsc -= 2; }
        if (bsc == 1 && d < p.e) { *d++ = *s++; }
    } else {
        // Backslashes are interpreted literally,
        // unless they immediately precede a quote:
        while (bsc > 0 && d < p.e) { *d++ = backslash; bsc--; }
    }
    return (args_pair_t){ .s = s, .d = d, .e = p.e };
}

static args_pair_t args_parse_quoted(args_pair_t p) {
    enum { quote = '"', backslash = '\\' };
    const char* s = p.s;
    char* d = p.d;
    swear(*s == quote);
    s++; // opening quote (skip)
    while (*s != 0x00) {
        if (*s == backslash) {
            p = args_parse_backslashes((args_pair_t){
                        .s = s, .d = d, .e = p.e });
            s = p.s; d = p.d;
        } else if (*s == quote && s[1] == quote) {
            // Within a quoted string, a pair of quote is
            // interpreted as a single escaped quote.
            if (d < p.e) { *d++ = *s++; }
            s++; // 1 for 2 quotes
        } else if (*s == quote) {
            s++; // closing quote (skip)
            break;
        } else if (d < p.e) {
            *d++ = *s++;
        }
    }
    return (args_pair_t){ .s = s, .d = d, .e = p.e };
}

static void args_parse(const char* s) {
    swear(s[0] != 0, "cannot parse empty string");
    swear(args.c == 0);
    swear(args.v == null);
    swear(args_memory == null);
    enum { quote = '"', backslash = '\\', tab = '\t', space = 0x20 };
    const int32_t len = (int)strlen(s);
    // Worst-case scenario (possible to optimize with dry run of parse)
    // at least 2 characters per token in "a b c d e" plush null at the end:
    const int32_t k = ((len + 2) / 2 + 1) * (int)sizeof(void*) + (int)sizeof(void*);
    const int32_t n = k + (len + 2) * (int)sizeof(char);
    fatal_if_not_zero(heap.allocate(null, &args_memory, n, true));
    args.c = 0;
    args.v = (const char**)args_memory;
    char* d = (char*)(((char*)args.v) + k);
    char* e = d + n; // end of memory
    // special rules for 1st argument:
    if (args.c < n) { args.v[args.c++] = d; }
    if (*s == quote) {
        s++;
        while (*s != 0x00 && *s != quote && d < e) { *d++ = *s++; }
        while (*s != 0x00) { s++; }
    } else {
        while (*s != 0x00 && *s != space && *s != tab && d < e) {
            *d++ = *s++;
        }
    }
    if (d < e) { *d++ = 0; }
    while (d < e) {
        while (*s == space || *s == tab) { s++; }
        if (*s == 0) { break; }
        if (*s == quote && s[1] == 0 && d < e) { // unbalanced single quote
            if (args.c < n) { args.v[args.c++] = d; } // spec does not say what to do
            *d++ = *s++;
        } else if (*s == quote) { // quoted arg
            if (args.c < n) { args.v[args.c++] = d; }
            args_pair_t p = args_parse_quoted(
                    (args_pair_t){ .s = s, .d = d, .e = e });
            s = p.s; d = p.d;
        } else { // non-quoted arg (that can have quoted strings inside)
            if (args.c < n) { args.v[args.c++] = d; }
            while (*s != 0) {
                if (*s == backslash) {
                    args_pair_t p = args_parse_backslashes(
                            (args_pair_t){ .s = s, .d = d, .e = e });
                    s = p.s; d = p.d;
                } else if (*s == quote) {
                    args_pair_t p = args_parse_quoted(
                            (args_pair_t){ .s = s, .d = d, .e = e });
                    s = p.s; d = p.d;
                } else if (*s == tab || *s == space) {
                    break;
                } else if (d < e) {
                    *d++ = *s++;
                }
            }
        }
        if (d < e) { *d++ = 0; }
    }
    if (args.c < n) {
        args.v[args.c] = null;
    }
    swear(args.c < n, "not enough memory - adjust guestimates");
    swear(d <= e, "not enough memory - adjust guestimates");
}

const char* args_basename(void) {
    static char basename[260];
    swear(args.c > 0);
    if (basename[0] == 0) {
        const char* s = args.v[0];
        const char* b = s;
        while (*s != 0) {
            if (*s == '\\' || *s == '/') { b = s + 1; }
            s++;
        }
        int32_t n = str.length(b);
        swear(n < countof(basename));
        strncpy(basename, b, countof(basename) - 1);
        char* d = basename + n - 1;
        while (d > basename && *d != '.') { d--; }
        if (*d == '.') { *d = 0x00; }
    }
    return basename;
}

static void args_fini(void) {
    heap.deallocate(null, args_memory); // can be null is parse() was not called
    args_memory = null;
    args.c = 0;
    args.v = null;
}

static void args_WinMain(void) {
    swear(args.c == 0 && args.v == null && args.env == null);
    swear(args_memory == null);
    const uint16_t* wcl = GetCommandLineW();
    int32_t n = (int32_t)wcslen(wcl);
    char* cl = null;
    fatal_if_not_zero(heap.allocate(null, &cl, n * 2 + 1, false));
    args_parse(str.utf16_utf8(cl, wcl));
    heap.deallocate(null, cl);
    args.env = _environ;
}

#ifdef UT_TESTS

// https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments
// Command-line input       argv[1]     argv[2]	    argv[3]
// "a b c" d e	            a b c       d           e
// "ab\"c" "\\" d           ab"c        \           d
// a\\\b d"e f"g h          a\\\b       de fg       h
// a\\\"b c d               a\"b        c           d
// a\\\\"b c" d e           a\\b c      d           e
// a"b"" c d                ab" c d

#ifndef __INTELLISENSE__ // confused data analysis

static void args_test_verify(const char* cl, int32_t expected, ...) {
    if (debug.verbosity.level >= debug.verbosity.trace) {
        traceln("cl: `%s`", cl);
    }
    int32_t argc = args.c;
    const char** argv = args.v;
    void* memory = args_memory;
    args.c = 0;
    args.v = null;
    args_memory = null;
    args_parse(cl);
    va_list vl;
    va_start(vl, expected);
    for (int32_t i = 0; i < expected; i++) {
        const char* s = va_arg(vl, const char*);
//      if (debug.verbosity.level >= debug.verbosity.trace) {
//          traceln("args.v[%d]: `%s` expected: `%s`", i, args.v[i], s);
//      }
        // Warning 6385: reading data outside of array
        const char* ai = _Pragma("warning(suppress:  6385)")args.v[i];
        swear(strcmp(ai, s) == 0, "args.v[%d]: `%s` expected: `%s`",
              i, ai, s);
    }
    va_end(vl);
    args.fini();
    // restore command line arguments:
    args.c = argc;
    args.v = argv;
    args_memory = memory;
}

#endif // __INTELLISENSE__

static void args_test(void) {
    // The first argument (args.v[0]) is treated specially.
    // It represents the program name. Because it must be a valid pathname,
    // parts surrounded by quote (") are allowed. The quote aren't included
    // in the args.v[0] output. The parts surrounded by quote prevent
    // interpretation of a space or tab character as the end of the argument.
    // The escaping rules don't apply.
    args_test_verify("\"c:\\foo\\bar\\snafu.exe\"", 1,
                     "c:\\foo\\bar\\snafu.exe");
    args_test_verify("c:\\foo\\bar\\snafu.exe", 1,
                     "c:\\foo\\bar\\snafu.exe");
    args_test_verify("foo.exe \"a b c\" d e", 4,
                     "foo.exe", "a b c", "d", "e");
    args_test_verify("foo.exe \"ab\\\"c\" \"\\\\\" d", 4,
                     "foo.exe", "ab\"c", "\\", "d");
    args_test_verify("foo.exe a\\\\\\b d\"e f\"g h", 4,
                     "foo.exe", "a\\\\\\b", "de fg", "h");
    args_test_verify("foo.exe a\\\\\\b d\"e f\"g h", 4,
                     "foo.exe", "a\\\\\\b", "de fg", "h");
    args_test_verify("foo.exe a\"b\"\" c d", 2, // unmatched quote
                     "foo.exe", "ab\" c d");
    // unbalanced quote and backslash:
    args_test_verify("foo.exe \"",     2, "foo.exe", "\"");
    args_test_verify("foo.exe \\",     2, "foo.exe", "\\");
    args_test_verify("foo.exe \\\\",   2, "foo.exe", "\\\\");
    args_test_verify("foo.exe \\\\\\", 2, "foo.exe", "\\\\\\");
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
}

#else

static void args_test(void) {}

#endif

args_if args = {
    .main         = args_main,
    .WinMain      = args_WinMain,
    .option_index = args_option_index,
    .remove_at    = args_remove_at,
    .option_bool  = args_option_bool,
    .option_int   = args_option_int,
    .option_str   = args_option_str,
    .basename     = args_basename,
    .fini         = args_fini,
    .test         = args_test
};
