#include "ut/ut.h"

// amalgamate \uh-MAL-guh-mayt\ verb. : to unite in or as if in a
// mixture of elements; especially : to merge into a single body.

enum { files_short_name = 260 };

static const char* name;
static char include_prefix[files_short_name]; // #include "<name>./"
static int32_t include_prefix_bytes;

const char* pragma_once = "#pragma once";
const char* c_begin     = "begin_c";
const char* c_end       = "end_c";

static int32_t pragma_once_bytes = sizeof(pragma_once) - 1;
static int32_t c_begin_bytes     = sizeof(c_begin) - 1;
static int32_t c_end_bytes       = sizeof(c_end) - 1;

static const char* basename(const char* filename) {
    const char* s = str.last_char(filename, '\\');
    if (s == null) { s = str.last_char(filename, '/'); }
    return s != null ? s + 1 : filename;
}

static int32_t usage(void) {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s <name>\n", basename(args.v[0]));
    fprintf(stderr, "Assumes src/name and inc/name folders exist\n");
    fprintf(stderr, "and inc/<name>/ contain <name>.h\n");
    fprintf(stderr, "\n");
    return 1;
}

typedef struct {
    char a[1024][files_short_name];
    int32_t n; // number of strings
} fns_t; // processed filenames

static fns_t fns;

static bool fns_contain(const char* fn) {
    for (int32_t i = 0; i < fns.n; i++) {
        if (str.equal(fns.a[i], fn)) { return true; }
    }
    return false;
}

static void fns_add(const char* fn) {
    assert(!fns_contain(fn));
    if (!fns_contain(fn)) {
        swear(fns.n < countof(fns.a) - 1);
        swear(str.length(fn) < countof(fns.a[0]) - 1);
        strprintf(fns.a[fns.n], "%s", fn);
        fns.n++;
    }
}

static int32_t line_bytes(const char* *s, const char* e) {
    assert(*s != e);
    const char* n = *s;
    while (n < e && *n >= 0x20) { n++; }
    uintptr_t k = (uintptr_t)(n - *s);
    while (n < e && *n != '\n') { n++; }
    assert(n <= e);
    *s = n == e ? n : n + 1;
    assert(k < UINT32_MAX);
    return (int32_t)k;
}

static bool str_starts(const char* s, const char* e, const char* pattern, int32_t bytes) {
    return bytes <= (int32_t)(uintptr_t)(e - s) &&
           str.starts_with(s, pattern);
}

static void definition(const char* filename, bool whole) {
//  traceln("filename: %s", filename);
    void* data = null;
    int64_t bytes = 0;
    fatal_if(mem.map_ro(filename, &data, &bytes) != 0);
    int32_t inside = 0;
    const char* s = (const char*)data;
    const char* e = (const char*)data + bytes;
    for (;;) {
        if (s >= e) { break; }
        const char* line = s;
        int32_t k = line_bytes(&s, e);
//traceln("\"%.*s\"\n", k, line);
//if (strstr(line, "begin_c") == line) { debug.breakpoint(); }
        if (str_starts(line, e, c_begin, c_begin_bytes)) {
            fatal_if(inside != 0, "%s: %.*s", filename, line, k);
            inside++;
        } else if (str_starts(line, e, c_end, c_end_bytes)) {
            fatal_if(inside != 1, "%s: %.*s", filename, line, k);
            inside--;
        } else if ((inside || whole) &&
            !str_starts(line, e, pragma_once, pragma_once_bytes) &&
            !str_starts(line, e, include_prefix, include_prefix_bytes)) {
            printf("%.*s\n", k, line);
        }
    }
    mem.unmap(data, bytes);
    printf("\n");
    fatal_if(inside != 0, "%s", filename);
}

static void header(const char* fn) {
    const char* underscores =
    "__________________________________________________________________________";
    int32_t i = (int32_t)(strlen(underscores) - strlen(fn)) / 2;
    int32_t j = (int32_t)(strlen(underscores) - i - strlen(fn));
    printf("// %.*s %s %.*s\n", i, underscores, fn, j, underscores);
}

static void include_file(const char* header_name, int32_t k, bool whole) {
    char fn[files_short_name];
    strprintf(fn, "%.*s", k, header_name);
    if (!fns_contain(fn)) {
        fns_add(fn);
        header(fn);
        char ifn[files_short_name];
        strprintf(ifn, "inc/%s/%s", name, fn);
        definition(ifn, whole);
    }
}

static void implementation(const char* filename) {
    void* data = null;
    int64_t bytes = 0;
    fatal_if(mem.map_ro(filename, &data, &bytes) != 0);
    const char* s = (const char*)data;
    const char* e = (const char*)data + bytes;
    for (;;) {
        if (s >= e) { break; }
        const char* line = s;
        int32_t k = line_bytes(&s, e);
        if (str.starts_with(line, include_prefix)) {
            char fn[files_short_name];
            const char* header = line + strlen(include_prefix);
            strprintf(fn, "%.*s", k - str.length(include_prefix) - 1, header);
            if (!fns_contain(fn)) {
                int32_t len = k - include_prefix_bytes;
                const char* q = str.first_char(header + 1, len, '"');
                include_file(header, (int32_t)(uintptr_t)(q - header), true);
            }
        } else {
            printf("%.*s\n", k, line);
        }
    }
    mem.unmap(data, bytes);
    printf("\n");
}

static void parse(char ext) {
    char fn[files_short_name];
    strprintf(fn, "%s.%c", name, ext);
    fns_add(fn);
    char filename[files_max_path];
    strprintf(filename, "%s/%s/%s", ext == 'h' ? "inc" : "src", name, fn);
    if (!files.exists(filename)) {
        fprintf(stderr, "file not found: \"%s\"", filename);
        runtime.exit(usage());
    }
    const char* suffix = ext == 'h' ? "definition" : "implementation";
    printf("#ifndef %s_%s\n", name, suffix);
    printf("#define %s_%s\n", name, suffix);
    printf("\n");
    void* data = null;
    int64_t bytes = 0;
    fatal_if(mem.map_ro(filename, &data, &bytes) != 0);
    bool first = true;
    int32_t inside = 0;
    const char* s = (const char*)data;
    const char* e = (const char*)data + bytes;
    for (;;) {
        if (s >= e) { break; }
        const char* line = s;
        int32_t k = line_bytes(&s, e);
        if (str_starts(line, e, include_prefix, include_prefix_bytes)) {
            const char* header = line + include_prefix_bytes;
            int32_t len = k - include_prefix_bytes;
            const char* q = str.first_char(header + 1, len, '"');
            include_file(header, (int32_t)(uintptr_t)(q - header), first);
            if (first && ext == 'h') {
                printf("\n");
                printf("begin_c\n");
                printf("\n");
            }
            first = false;
        } else if (str_starts(line, e, c_begin, c_begin_bytes)) {
            fatal_if(inside != 0, "%s: %.*s", filename, line, k);
            inside++;
        } else if (str_starts(line, e, c_end, c_end_bytes)) {
            fatal_if(inside != 1, "%s: %.*s", filename, line, k);
            inside--;
        } else if (inside &&
            !str_starts(line, e, pragma_once, pragma_once_bytes)) {
            printf("%.*s\n", k, line);
        }
    }
    mem.unmap(data, bytes);
    fatal_if(inside != 0, "%s", filename);
    if (first && ext == 'h') {
        printf("\n");
        printf("end_c\n");
        printf("\n");
    } else {
        char src[files_max_path];
        strprintf(src, "src/%s", name);
        folder_t* f = null;
        fatal_if(folders.open(&f, src) != 0);
        int32_t count = folders.count(f);
        for (int32_t i = 0; i < count; i++) {
            const char* fname = folders.name(f, i);
            header(fname);
            char src_file[files_short_name];
            strprintf(src_file, "src/%s/%s", name, fname);
            implementation(src_file);
        }
        folders.close(f);
    }
    printf("#endif // %s_%s\n", name, suffix);
}

static char folder[files_max_path];

static void msvc_folder_up2(const char* argv0) {
    // On github CI the bin/release/foo.exe is usually
    // started at the root of the repository (unless some
    // cd or pushd was invoked)
    // In MSVC by default executables start at projects
    // folder which is one or two level deep from the root
    strcpy(folder, argv0);
    char* last = strrchr(folder, '\\');
    fatal_if_null(last);
    *last = 0;
    if (strendswith(folder, "bin\\release") ||
        strendswith(folder, "bin\\release") ||
        strendswith(folder, "bin\\debug") ||
        strendswith(folder, "bin\\debug")) {
        char cd[files_max_path];
        strprintf(cd, "%s\\..\\..", folder);
        fatal_if(folders.setcwd(cd) != 0);
    }
}

int main(int unused(argc), const char* argv[]) {
    msvc_folder_up2(argv[0]); // msvc debugging convenience
    fatal_if(folders.cwd(folder, countof(folder)) != 0);
//  traceln("%s", folder);
    if (argc < 2) { runtime.exit(usage()); }
    name = argv[1];
    char inc[files_max_path];
    strprintf(inc, "inc/%s", name);
    char src[files_max_path];
    strprintf(src, "src/%s", name);
    if (!files.is_folder(inc) || !files.is_folder(src)) {
        fprintf(stderr, "folders not found: \"%s\" or \"%s\"", inc, src);
        runtime.exit(usage());
    }
    pragma_once_bytes = str.length(pragma_once);
    c_begin_bytes     = str.length(c_begin);
    c_end_bytes       = str.length(c_end);
    strprintf(include_prefix, "#include \"%s/", name);
    include_prefix_bytes = str.length(include_prefix);
    parse('h');
    parse('c');
    return 0;
}
