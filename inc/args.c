#include "args.h"
#include "vigil.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

begin_c

#pragma push_macro("ns") // namespace
#pragma push_macro("fn") // function

#define ns(name) args_ ## name
#define fn(type, name) static type ns(name)

fn(int, option_index)(int argc, const char* argv[], const char* option) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) { break; } // no options after '--'
        if (strcmp(argv[i], option) == 0) { return i; }
    }
    return -1;
}

fn(int, remove_at)(int ix, int argc, const char* argv[]) { // returns new argc
    assert(0 < argc);
    assert(0 < ix && ix < argc); // cannot remove argv[0]
    for (int i = ix; i < argc; i++) {
        argv[i] = argv[i+1];
    }
    argv[argc - 1] = "";
    return argc - 1;
}

fn(bool, option_bool)(int *argc, const char* argv[], const char* option) {
    int ix = ns(option_index)(*argc, argv, option);
    if (ix > 0) {
        *argc = ns(remove_at)(ix, *argc, argv);
    }
    return ix > 0;
}

fn(bool, option_int)(int *argc, const char* argv[], const char* option,
        int64_t *value) {
    int ix = ns(option_index)(*argc, argv, option);
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
        *argc = ns(remove_at)(ix, *argc, argv); // remove option
        *argc = ns(remove_at)(ix, *argc, argv); // remove following number
    }
    return ix > 0;
}

fn(const char*, option_str)(int *argc, const char* argv[], const char* option) {
    int ix = ns(option_index)(*argc, argv, option);
    const char* s = null;
    if (ix > 0 && ix < *argc - 1) {
        s = argv[ix + 1];
    } else {
        ix = -1;
    }
    if (ix > 0) {
        *argc = ns(remove_at)(ix, *argc, argv); // remove option
        *argc = ns(remove_at)(ix, *argc, argv); // remove following string
    }
    return ix > 0 ? s : null;
}

static const char ns(backslash) = '\\';
static const char ns(quote) = '\"';

fn(char, next_char)(const char** cl, int* escaped) {
    char ch = **cl;
    (*cl)++;
    *escaped = false;
    if (ch == ns(backslash)) {
        if (**cl == ns(backslash)) {
            (*cl)++;
            *escaped = true;
        } else if (**cl == ns(quote)) {
            ch = ns(quote);
            (*cl)++;
            *escaped = true;
        } else { /* keep the backslash and copy it into the resulting argument */ }
    }
    return ch;
}

fn(int, parse)(const char* cl, const char** argv, char* buff) {
    int escaped = 0;
    int argc = 0;
    int j = 0;
    char ch = args_next_char(&cl, &escaped);
    while (ch != 0) {
        while (isspace(ch)) { ch = args_next_char(&cl, &escaped); }
        if (ch == 0) { break; }
        argv[argc++] = buff + j;
        if (ch == ns(quote)) {
            ch = args_next_char(&cl, &escaped);
            while (ch != 0) {
                if (ch == ns(quote) && !escaped) { break; }
                buff[j++] = ch;
                ch = args_next_char(&cl, &escaped);
            }
            buff[j++] = 0;
            if (ch == 0) { break; }
            ch = args_next_char(&cl, &escaped); // skip closing quote maerk
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
    .option_index = ns(option_index),
    .remove_at    = ns(remove_at),
    .option_bool  = ns(option_bool),
    .option_int   = ns(option_int),
    .option_str   = ns(option_str),
    .parse        = ns(parse)
};

#pragma pop_macro("fn") // function
#pragma pop_macro("ns") // namespace

end_c
