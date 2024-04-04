#include "runtime/runtime.h"

// Terminology: "quote" in the code and comments below
// actually refers to "double quote mark" and used for brevity.

static int32_t args_option_index(int32_t argc, const char* argv[], const char* option) {
    for (int32_t i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) { break; } // no options after '--'
        if (strcmp(argv[i], option) == 0) { return i; }
    }
    return -1;
}

static int32_t args_remove_at(int32_t ix, int32_t argc, const char* argv[]) { // returns new argc
    assert(0 < argc);
    assert(0 < ix && ix < argc); // cannot remove argv[0]
    for (int32_t i = ix; i < argc; i++) {
        argv[i] = argv[i+1];
    }
    argv[argc - 1] = "";
    return argc - 1;
}

static bool args_option_bool(int32_t *argc, const char* argv[], const char* option) {
    int32_t ix = args_option_index(*argc, argv, option);
    if (ix > 0) {
        *argc = args_remove_at(ix, *argc, argv);
    }
    return ix > 0;
}

static bool args_option_int(int32_t *argc, const char* argv[], const char* option,
        int64_t *value) {
    int32_t ix = args_option_index(*argc, argv, option);
    if (ix > 0 && ix < *argc - 1) {
        const char* s = argv[ix + 1];
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
        *argc = args_remove_at(ix, *argc, argv); // remove option
        *argc = args_remove_at(ix, *argc, argv); // remove following number
    }
    return ix > 0;
}

static const char* args_option_str(int32_t *argc, const char* argv[],
        const char* option) {
    int32_t ix = args_option_index(*argc, argv, option);
    const char* s = null;
    if (ix > 0 && ix < *argc - 1) {
        s = argv[ix + 1];
    } else {
        ix = -1;
    }
    if (ix > 0) {
        *argc = args_remove_at(ix, *argc, argv); // remove option
        *argc = args_remove_at(ix, *argc, argv); // remove following string
    }
    return ix > 0 ? s : null;
}

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

typedef struct { const char* s; char* d; } args_pair_t;

static args_pair_t args_parse_backslashes(args_pair_t p) {
    enum { quote = '"', backslash = '\\' };
    const char* s = p.s;
    char* d = p.d;
    swear(*s == backslash);
    int32_t bsc = 0; // number of backslashes
    while (*s == backslash) { s++; bsc++; }
    if (*s == quote) {
        while (bsc > 1) { *d++ = backslash; bsc -= 2; }
        if (bsc == 1) { *d++ = *s++; }
    } else {
        // Backslashes are interpreted literally,
        // unless they immediately precede a quote:
        while (bsc > 0) { *d++ = backslash; bsc--; }
    }
    return (args_pair_t){ .s = s, .d = d };
}

static args_pair_t args_parse_quoted(args_pair_t p) {
    enum { quote = '"', backslash = '\\' };
    const char* s = p.s;
    char* d = p.d;
    swear(*s == quote);
    s++; // opening quote (skip)
    while (*s != 0x00) {
        if (*s == backslash) {
            p = args_parse_backslashes((args_pair_t){ .s = s, .d = d });
            s = p.s; d = p.d;
        } else if (*s == quote && s[1] == quote) {
            // Within a quoted string, a pair of quote is
            // interpreted as a single escaped quote.
            *d++ = *s++;
            s++; // 1 for 2 quotes
        } else if (*s == quote) {
            s++; // closing quote (skip)
            break;
        } else {
            *d++ = *s++;
        }
    }
    return (args_pair_t){ .s = s, .d = d };
}

static int32_t args_parse(const char* s, const char** argv, char* d) {
    swear(s[0] != 0, "cannot parse empty string");
    enum { quote = '"', backslash = '\\', tab = '\t', space = 0x20 };
    int32_t argc = 0;
    // special rules for 1st argument:
    argv[argc++] = d;
    if (*s == quote) {
        s++;
        while (*s != 0x00 && *s != quote) { *d++ = *s++; }
        while (*s != 0x00) { s++; }
    } else {
        while (*s != 0x00 && *s != space && *s != tab) { *d++ = *s++; }
    }
    *d++ = 0;
    for (;;) {
        while (*s == space || *s == tab) { s++; }
        if (*s == 0) { break; }
        if (*s == quote && s[1] == 0) { // unbalanced single quote
            argv[argc++] = d; // spec does not say what to do
            *d++ = *s++;
        } else if (*s == quote) { // quoted arg
            argv[argc++] = d;
            args_pair_t p = args_parse_quoted(
                    (args_pair_t){ .s = s, .d = d });
            s = p.s; d = p.d;
        } else { // non-quoted arg (that can have quoted strings inside)
            argv[argc++] = d;
            while (*s != 0) {
                if (*s == backslash) {
                    args_pair_t p = args_parse_backslashes(
                            (args_pair_t){ .s = s, .d = d });
                    s = p.s; d = p.d;
                } else if (*s == quote) {
                    args_pair_t p = args_parse_quoted(
                            (args_pair_t){ .s = s, .d = d });
                    s = p.s; d = p.d;
                } else if (*s == tab || *s == space) {
                    break;
                } else {
                    *d++ = *s++;
                }
            }
        }
        *d++ = 0;
    }
    argv[argc] = null;
    return argc;
}

#ifdef RUNTIME_TESTS

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
    const int32_t len = (int)strlen(cl);
    // at least 2 characters per token in "a b c d e" plush null at the end:
    const int32_t k = ((len + 2) / 2 + 1) * (int)sizeof(void*) + (int)sizeof(void*);
    const int32_t n = k + (len + 2) * (int)sizeof(char);
    const char** argv = (const char**)stackalloc(n);
    memset(argv, 0, n);
    char* buff = (char*)(((char*)argv) + k);
    int32_t argc = args.parse(cl, argv, buff);
    swear(argc == expected, "argc: %d expected: %d", argc, expected);
    va_list vl;
    va_start(vl, expected);
    for (int32_t i = 0; i < expected; i++) {
        const char* s = va_arg(vl, const char*);
        if (debug.verbosity.level >= debug.verbosity.trace) {
            traceln("argv[%d]: `%s` expected: `%s`", i, argv[i], s);
        }
        #pragma warning(push)
        #pragma warning(disable: 6385) // reading data outside of array
        swear(str.equal(argv[i], s), "argv[%d]: `%s` expected: `%s`",
              i, argv[i], s);
        #pragma warning(pop)
    }
    va_end(vl);
}

#endif // __INTELLISENSE__

static void args_test(void) {
    // The first argument (argv[0]) is treated specially.
    // It represents the program name. Because it must be a valid pathname,
    // parts surrounded by quote (") are allowed. The quote aren't included
    // in the argv[0] output. The parts surrounded by quote prevent interpretation
    // of a space or tab character as the end of the argument.
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
    if (debug.verbosity.level > debug.verbosity.quiet) {
        traceln("done");
    }
}

#else

static void args_test(void) {}

#endif

args_if args = {
    .option_index = args_option_index,
    .remove_at    = args_remove_at,
    .option_bool  = args_option_bool,
    .option_int   = args_option_int,
    .option_str   = args_option_str,
    .parse        = args_parse,
    .test         = args_test
};

#ifdef WINDOWS

static_init(args) {
    args.c = __argc;
    args.v = __argv;
    args.env = _environ;
}

#endif
