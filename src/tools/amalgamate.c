#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dirent.h"

#if defined(__GNUC__) || defined(__clang__) // TODO: remove and fix code
#pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma GCC diagnostic ignored "-Wfour-char-constants"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Waddress-of-packed-member"
#pragma GCC diagnostic ignored "-Wused-but-marked-unused" // because in debug only
#endif

#define null NULL

#ifndef countof
    #define ut_countof(a) ((int)(sizeof(a) / sizeof((a)[0])))
#endif

#define strequ(s1, s2) (strcmp((s1), (s2)) == 0)

static const char* exe;
static const char* name;
static int32_t strlen_name;
static char inc[256];
static char src[256];
static char mem[1024 * 1023];
static char* brk = mem;

#define ut_fatal_if(x, ...) do {                                   \
    if (x) {                                                    \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); \
        fprintf(stderr, "" __VA_ARGS__);                        \
        fprintf(stderr, "\nFATAL\n");                           \
        exit(1);                                                \
    }                                                           \
} while (0)

static const char* basename(const char* filename) {
    const char* s = strrchr(filename, '\\');
    if (s == null) { s = strrchr(filename, '/'); }
    return s != null ? s + 1 : filename;
}

static char* dup(const char* s) { // strdup() like to avoid leaks reporting
    int n = (int)strlen(s) + 1;
    ut_fatal_if(brk + n > mem + sizeof(mem), "out of memory");
    char* c = (char*)memcpy(brk, s, (size_t)n);
    brk += n;
    return c;
}

static char* concat(const char* s1, const char* s2) {
    int n1 = (int)strlen(s1);
    int n2 = (int)strlen(s2);
    ut_fatal_if(brk + n1 + n2 + 1 > mem + sizeof(mem), "out of memory");
    char* c = brk;
    memcpy((char*)memcpy(brk, s1, (size_t)n1) + n1, s2, (size_t)n2 + 1);
    brk += n1 + n2 + 1;
    return c;
}

static bool ends_with(const char* s1, const char* s2) {
    int32_t n1 = (int)strlen(s1);
    int32_t n2 = (int)strlen(s2);
    return n1 >= n2 && strequ(s1 + n1 - n2, s2);
}

typedef struct { const char* a[1024]; int32_t n; } set_t;

static set_t files;
static set_t includes;

static bool set_has(set_t* set, const char* s) {
    for (int32_t i = 0; i < set->n; i++) {
        if (strequ(set->a[i], s)) { return true; }
    }
    return false;
}

static void set_add(set_t* set, const char* s) {
    assert(!set_has(set, s));
    if (!set_has(set, s)) {
        ut_fatal_if(set->n == ut_countof(set->a), "too many files");
        set->a[set->n] = dup(s);
        set->n++;
    }
}

static int32_t usage(void) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s <name>\n", exe);
    fprintf(stderr, "Assumes src/name and inc/name folders exist\n");
    fprintf(stderr, "and inc/<name>/ contain <name>.h\n");
    fprintf(stderr, "\n");
    return 1;
}

static void tail_trim(char* s) {
    char* p = s + strlen(s) - 1;
    while (p >= s && *p < 0x20) { *p-- = 0x00; }
}

static void divider(const char* fn) {
    char underscores[40] = {0};
    memset(underscores, '_', ut_countof(underscores) - 1);
    int32_t i = (int)(74 - strlen(fn)) / 2;
    int32_t j = (int)(74 - i - (int)strlen(fn));
    printf("// %.*s %s %.*s\n\n", i, underscores, fn, j, underscores);
}

static const char* include(char* s) {
    char fn[256] = {0};
    const char* include = "#include\x20\"";
    if (strstr(s, include) == s) {
        s += strlen(include);
        const char* q = strchr(s, '"');
        if (q != null) {
            snprintf(fn, ut_countof(fn) - 1, "%.*s", (int)(q - s), s);
            return strstr(fn, name) == fn && fn[strlen_name] == '/' ?
                   dup(fn + strlen_name + 1) : null;
        }
    }
    return null;
}


static bool already_included(const char* s) {
    const char* include = "#include\x20\"";
    if (strstr(s, include) != s) { return false; }
    if (set_has(&includes, s)) { return true; }
    set_add(&includes, s);
    return false;
}

static bool ignore(const char* s) {
    return strequ(s, "#pragma once") || already_included(s);
}

static void parse(const char* fn) {
    FILE* f = fopen(fn, "r");
    ut_fatal_if(f == null, "file not found: `%s`", fn);
    static char line[16 * 1024];
    bool first = true;
    while (fgets(line, ut_countof(line) - 1, f) != null) {
        tail_trim(line);
        const char* in = include(line);
        if (in != null) {
            if (!set_has(&files, in)) {
                set_add(&files, in);
                parse(concat(inc, concat("/", in)));
            }
        } else if (ends_with(fn, ".c") || !ignore(line)) {
            if (first && line[0] != 0) {
                divider(fn + 5 + strlen_name); first = false;
            }
            printf("%s\n", line);
        }
    }
    fclose(f);
}

static void definition(void) {
    printf("#ifndef %s_definition\n", name);
    printf("#define %s_definition\n", name);
    printf("\n");
    parse(concat(inc, concat("/", concat(name, ".h"))));
    printf("\n");
    printf("#endif // %s_definition\n", name);
    const char* name_h = concat(name, ".h");
    // because name.h is fully processed do not include it again:
    if (!set_has(&files, name_h)) { set_add(&files, concat(name, ".h")); }
}

static void implementation(void) {
    printf("\n");
    printf("#ifdef %s_implementation\n", name);
    DIR* d = opendir(src);
    ut_fatal_if(d == null, "folder not found: `%s`", src);
    struct dirent* e = readdir(d);
    while (e != null) {
        if (ends_with(e->d_name, ".c")) {
            parse(concat(src, concat("/", e->d_name)));
        }
        e = readdir(d);
    }
    ut_fatal_if(closedir(d) != 0);
    printf("\n");
    printf("#endif // %s_implementation\n", name);
    printf("\n");
}

int main(int argc, const char* argv[]) {
    exe = basename(argv[0]);
    if (argc < 2) { exit(usage()); }
    name = argv[1];
    strlen_name = (int)strlen(name);
    snprintf(inc, ut_countof(inc) - 1, "inc/%s", name);
    snprintf(src, ut_countof(inc) - 1, "src/%s", name);
    definition();
    implementation();
    return 0;
}
