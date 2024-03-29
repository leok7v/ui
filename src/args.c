#include "rt.h"

static int32_t args_option_index(int argc, const char* argv[], const char* option) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) { break; } // no options after '--'
        if (strcmp(argv[i], option) == 0) { return i; }
    }
    return -1;
}

static int32_t args_remove_at(int ix, int argc, const char* argv[]) { // returns new argc
    assert(0 < argc);
    assert(0 < ix && ix < argc); // cannot remove argv[0]
    for (int i = ix; i < argc; i++) {
        argv[i] = argv[i+1];
    }
    argv[argc - 1] = "";
    return argc - 1;
}

static bool args_option_bool(int *argc, const char* argv[], const char* option) {
    int ix = args_option_index(*argc, argv, option);
    if (ix > 0) {
        *argc = args_remove_at(ix, *argc, argv);
    }
    return ix > 0;
}

static bool args_option_int(int *argc, const char* argv[], const char* option,
        int64_t *value) {
    int ix = args_option_index(*argc, argv, option);
    if (ix > 0 && ix < *argc - 1) {
        const char* s = argv[ix + 1];
        int base = (strstr(s, "0x") == s || strstr(s, "0X") == s) ? 16 : 10;
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

static const char* args_option_str(int *argc, const char* argv[],
        const char* option) {
    int ix = args_option_index(*argc, argv, option);
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

static const char args_backslash = '\\';
static const char args_quote = '\"';

static char args_next_char(const char** cl, int* escaped) {
    char ch = **cl;
    (*cl)++;
    *escaped = false;
    if (ch == args_backslash) {
        if (**cl == args_backslash) {
            (*cl)++;
            *escaped = true;
        } else if (**cl == args_quote) {
            ch = args_quote;
            (*cl)++;
            *escaped = true;
        } else {
            // keep the backslash and copy it into the resulting argument
        }
    }
    return ch;
}

static int32_t args_parse(const char* cl, const char** argv, char* buff) {
    int escaped = 0;
    int argc = 0;
    int j = 0;
    char ch = args_next_char(&cl, &escaped);
    while (ch != 0) {
        while (isspace(ch)) { ch = args_next_char(&cl, &escaped); }
        if (ch == 0) { break; }
        argv[argc++] = buff + j;
        if (ch == args_quote) {
            ch = args_next_char(&cl, &escaped);
            while (ch != 0) {
                if (ch == args_quote && !escaped) { break; }
                buff[j++] = ch;
                ch = args_next_char(&cl, &escaped);
            }
            buff[j++] = 0;
            if (ch == 0) { break; }
            ch = args_next_char(&cl, &escaped); // skip closing quote mark
        } else {
            while (ch != 0 && !isspace(ch)) {
                buff[j++] = ch;
                ch = args_next_char(&cl, &escaped);
            }
            buff[j++] = 0;
        }
    }
    return argc;
}

args_if args = {
    .option_index = args_option_index,
    .remove_at    = args_remove_at,
    .option_bool  = args_option_bool,
    .option_int   = args_option_int,
    .option_str   = args_option_str,
    .parse        = args_parse
};
