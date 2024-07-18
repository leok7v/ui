#include "ut/ut.h"
#include "ut/ut_win32.h"

static void* ut_args_memory;

static void ut_args_main(int32_t argc, const char* argv[], const char** env) {
    swear(ut_args.c == 0 && ut_args.v == null && ut_args.env == null);
    swear(ut_args_memory == null);
    ut_args.c = argc;
    ut_args.v = argv;
    ut_args.env = env;
}

static int32_t ut_args_option_index(const char* option) {
    for (int32_t i = 1; i < ut_args.c; i++) {
        if (strcmp(ut_args.v[i], "--") == 0) { break; } // no options after '--'
        if (strcmp(ut_args.v[i], option) == 0) { return i; }
    }
    return -1;
}

static void ut_args_remove_at(int32_t ix) {
    // returns new argc
    assert(0 < ut_args.c);
    assert(0 < ix && ix < ut_args.c); // cannot remove ut_args.v[0]
    for (int32_t i = ix; i < ut_args.c; i++) {
        ut_args.v[i] = ut_args.v[i + 1];
    }
    ut_args.v[ut_args.c - 1] = "";
    ut_args.c--;
}

static bool ut_args_option_bool(const char* option) {
    int32_t ix = ut_args_option_index(option);
    if (ix > 0) { ut_args_remove_at(ix); }
    return ix > 0;
}

static bool ut_args_option_int(const char* option, int64_t *value) {
    int32_t ix = ut_args_option_index(option);
    if (ix > 0 && ix < ut_args.c - 1) {
        const char* s = ut_args.v[ix + 1];
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
        ut_args_remove_at(ix); // remove option
        ut_args_remove_at(ix); // remove following number
    }
    return ix > 0;
}

static const char* ut_args_option_str(const char* option) {
    int32_t ix = ut_args_option_index(option);
    const char* s = null;
    if (ix > 0 && ix < ut_args.c - 1) {
        s = ut_args.v[ix + 1];
    } else {
        ix = -1;
    }
    if (ix > 0) {
        ut_args_remove_at(ix); // remove option
        ut_args_remove_at(ix); // remove following string
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

typedef struct { const char* s; char* d; const char* e; } ut_args_pair_t;

static ut_args_pair_t ut_args_parse_backslashes(ut_args_pair_t p) {
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
    return (ut_args_pair_t){ .s = s, .d = d, .e = p.e };
}

static ut_args_pair_t ut_args_parse_quoted(ut_args_pair_t p) {
    enum { quote = '"', backslash = '\\' };
    const char* s = p.s;
    char* d = p.d;
    swear(*s == quote);
    s++; // opening quote (skip)
    while (*s != 0x00) {
        if (*s == backslash) {
            p = ut_args_parse_backslashes((ut_args_pair_t){
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
    return (ut_args_pair_t){ .s = s, .d = d, .e = p.e };
}

static void ut_args_parse(const char* s) {
    swear(s[0] != 0, "cannot parse empty string");
    swear(ut_args.c == 0);
    swear(ut_args.v == null);
    swear(ut_args_memory == null);
    enum { quote = '"', backslash = '\\', tab = '\t', space = 0x20 };
    const int32_t len = (int32_t)strlen(s);
    // Worst-case scenario (possible to optimize with dry run of parse)
    // at least 2 characters per token in "a b c d e" plush null at the end:
    const int32_t k = ((len + 2) / 2 + 1) * (int32_t)sizeof(void*) + (int32_t)sizeof(void*);
    const int32_t n = k + (len + 2) * (int32_t)sizeof(char);
    ut_fatal_if_error(ut_heap.allocate(null, &ut_args_memory, n, true));
    ut_args.c = 0;
    ut_args.v = (const char**)ut_args_memory;
    char* d = (char*)(((char*)ut_args.v) + k);
    char* e = d + n; // end of memory
    // special rules for 1st argument:
    if (ut_args.c < n) { ut_args.v[ut_args.c++] = d; }
    if (*s == quote) {
        s++;
        while (*s != 0x00 && *s != quote && d < e) { *d++ = *s++; }
        if (*s == quote) { // // closing quote
            s++; // skip closing quote
            *d++ = 0x00;
        } else {
            while (*s != 0x00) { s++; }
        }
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
            if (ut_args.c < n) { ut_args.v[ut_args.c++] = d; } // spec does not say what to do
            *d++ = *s++;
        } else if (*s == quote) { // quoted arg
            if (ut_args.c < n) { ut_args.v[ut_args.c++] = d; }
            ut_args_pair_t p = ut_args_parse_quoted(
                    (ut_args_pair_t){ .s = s, .d = d, .e = e });
            s = p.s; d = p.d;
        } else { // non-quoted arg (that can have quoted strings inside)
            if (ut_args.c < n) { ut_args.v[ut_args.c++] = d; }
            while (*s != 0) {
                if (*s == backslash) {
                    ut_args_pair_t p = ut_args_parse_backslashes(
                            (ut_args_pair_t){ .s = s, .d = d, .e = e });
                    s = p.s; d = p.d;
                } else if (*s == quote) {
                    ut_args_pair_t p = ut_args_parse_quoted(
                            (ut_args_pair_t){ .s = s, .d = d, .e = e });
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
    if (ut_args.c < n) {
        ut_args.v[ut_args.c] = null;
    }
    swear(ut_args.c < n, "not enough memory - adjust guestimates");
    swear(d <= e, "not enough memory - adjust guestimates");
}

static const char* ut_args_basename(void) {
    static char basename[260];
    swear(ut_args.c > 0);
    if (basename[0] == 0) {
        const char* s = ut_args.v[0];
        const char* b = s;
        while (*s != 0) {
            if (*s == '\\' || *s == '/') { b = s + 1; }
            s++;
        }
        int32_t n = ut_str.len(b);
        swear(n < ut_countof(basename));
        strncpy(basename, b, ut_countof(basename) - 1);
        char* d = basename + n - 1;
        while (d > basename && *d != '.') { d--; }
        if (*d == '.') { *d = 0x00; }
    }
    return basename;
}

static void ut_args_fini(void) {
    ut_heap.deallocate(null, ut_args_memory); // can be null is parse() was not called
    ut_args_memory = null;
    ut_args.c = 0;
    ut_args.v = null;
}

static void ut_args_WinMain(void) {
    swear(ut_args.c == 0 && ut_args.v == null && ut_args.env == null);
    swear(ut_args_memory == null);
    const uint16_t* wcl = GetCommandLineW();
    int32_t n = (int32_t)ut_str.len16(wcl);
    char* cl = null;
    ut_fatal_if_error(ut_heap.allocate(null, (void**)&cl, n * 2 + 1, false));
    ut_str.utf16to8(cl, n * 2 + 1, wcl, -1);
    ut_args_parse(cl);
    ut_heap.deallocate(null, cl);
    ut_args.env = (const char**)(void*)_environ;
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

static void ut_args_test_verify(const char* cl, int32_t expected, ...) {
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
        ut_println("cl: `%s`", cl);
    }
    int32_t argc = ut_args.c;
    const char** argv = ut_args.v;
    void* memory = ut_args_memory;
    ut_args.c = 0;
    ut_args.v = null;
    ut_args_memory = null;
    ut_args_parse(cl);
    va_list va;
    va_start(va, expected);
    for (int32_t i = 0; i < expected; i++) {
        const char* s = va_arg(va, const char*);
//      if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
//          ut_println("ut_args.v[%d]: `%s` expected: `%s`", i, ut_args.v[i], s);
//      }
        // Warning 6385: reading data outside of array
        const char* ai = _Pragma("warning(suppress:  6385)")ut_args.v[i];
        swear(strcmp(ai, s) == 0, "ut_args.v[%d]: `%s` expected: `%s`",
              i, ai, s);
    }
    va_end(va);
    ut_args.fini();
    // restore command line arguments:
    ut_args.c = argc;
    ut_args.v = argv;
    ut_args_memory = memory;
}

#endif // __INTELLISENSE__

static void ut_args_test(void) {
    // The first argument (ut_args.v[0]) is treated specially.
    // It represents the program name. Because it must be a valid pathname,
    // parts surrounded by quote (") are allowed. The quote aren't included
    // in the ut_args.v[0] output. The parts surrounded by quote prevent
    // interpretation of a space or tab character as the end of the argument.
    // The escaping rules don't apply.
    ut_args_test_verify("\"c:\\foo\\bar\\snafu.exe\"", 1,
                     "c:\\foo\\bar\\snafu.exe");
    ut_args_test_verify("c:\\foo\\bar\\snafu.exe", 1,
                     "c:\\foo\\bar\\snafu.exe");
    ut_args_test_verify("foo.exe \"a b c\" d e", 4,
                     "foo.exe", "a b c", "d", "e");
    ut_args_test_verify("foo.exe \"ab\\\"c\" \"\\\\\" d", 4,
                     "foo.exe", "ab\"c", "\\", "d");
    ut_args_test_verify("foo.exe a\\\\\\b d\"e f\"g h", 4,
                     "foo.exe", "a\\\\\\b", "de fg", "h");
    ut_args_test_verify("foo.exe a\\\\\\b d\"e f\"g h", 4,
                     "foo.exe", "a\\\\\\b", "de fg", "h");
    ut_args_test_verify("foo.exe a\"b\"\" c d", 2, // unmatched quote
                     "foo.exe", "ab\" c d");
    // unbalanced quote and backslash:
    ut_args_test_verify("foo.exe \"",     2, "foo.exe", "\"");
    ut_args_test_verify("foo.exe \\",     2, "foo.exe", "\\");
    ut_args_test_verify("foo.exe \\\\",   2, "foo.exe", "\\\\");
    ut_args_test_verify("foo.exe \\\\\\", 2, "foo.exe", "\\\\\\");
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_println("done"); }
}

#else

static void ut_args_test(void) {}

#endif

ut_args_if ut_args = {
    .main         = ut_args_main,
    .WinMain      = ut_args_WinMain,
    .option_index = ut_args_option_index,
    .remove_at    = ut_args_remove_at,
    .option_bool  = ut_args_option_bool,
    .option_int   = ut_args_option_int,
    .option_str   = ut_args_option_str,
    .basename     = ut_args_basename,
    .fini         = ut_args_fini,
    .test         = ut_args_test
};
