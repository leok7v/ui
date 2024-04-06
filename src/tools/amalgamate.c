#include "runtime/runtime.h"

// amalgamate \uh-MAL-guh-mayt\ verb. : to unite in or as if in a
// mixture of elements; especially : to merge into a single body.

static const char* name;
static char include_name[1024];

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
    char a[1024][128];
    int32_t n; // number of strings
} includes_t;

static includes_t includes;

static bool includes_contain(const char* fn) {
    for (int32_t i = 0; i < includes.n; i++) {
        if (str.equal(includes.a[i], fn)) { return true; }
    }
    return false;
}

static void includes_add(const char* fn) {
    if (!includes_contain(fn)) {
        swear(includes.n < countof(includes.a) - 1);
        strprintf(includes.a[includes.n], "%s", fn);
        includes.n++;
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

static void cat_header_file(const char* filename) {
    void* data = null;
    int64_t bytes = 0;
    fatal_if(mem.map_ro(filename, &data, &bytes) != 0);
    const char* s = (const char*)data;
    const char* e = (const char*)data + bytes;
    for (;;) {
        if (s >= e) { break; }
        const char* line = s;
        int32_t k = line_bytes(&s, e);
        if (!str.starts_with(line, "#pragma once") &&
            !str.starts_with(line, include_name)) {
            printf("%.*s\n", k, line);
        }
    }
    mem.unmap(data, bytes);
    printf("\n");
}

static void header(const char* fn) {
    const char* underscores =
    "__________________________________________________________________________";
    int32_t i = (int32_t)(strlen(underscores) - strlen(fn)) / 2;
    int32_t j = (int32_t)(strlen(underscores) - i - strlen(fn));
    printf("// %.*s %s %.*s\n", i, underscores, fn, j, underscores);
}

static void include_file(const char* filename, int32_t k) {
    char fn[1024];
    strprintf(fn, "%.*s", k, filename);
    includes_add(fn);
    header(fn);
    char include_file[1024];
    strprintf(include_file, "inc/%s/%s", name, fn);
    cat_header_file(include_file);
}

static void process_root(const char* root) {
    char root_h[1024];
    strprintf(root_h, "%s.h", name);
    includes_add(root_h);
    printf("#ifndef %s_definition\n", name);
    printf("#define %s_definition\n", name);
    printf("\n");
    void* data = null;
    int64_t bytes = 0;
    fatal_if(mem.map_ro(root, &data, &bytes) != 0);
    const char* s = (const char*)data;
    const char* e = (const char*)data + bytes;
    for (;;) {
        if (s >= e) { break; }
        const char* line = s;
        int32_t k = line_bytes(&s, e);
        if (str.starts_with(line, "#pragma once")) {
            // skip
        } else if (str.starts_with(line, include_name)) {
            const char* header = line + strlen(include_name);
            include_file(header, k - str.length(include_name) - 1);
        } else {
            printf("%.*s\n", k, line);
        }
    }
    mem.unmap(data, bytes);
    printf("#endif // %s_definition\n", name);
}

static void cat_source_file(const char* filename) {
    void* data = null;
    int64_t bytes = 0;
    fatal_if(mem.map_ro(filename, &data, &bytes) != 0);
    const char* s = (const char*)data;
    const char* e = (const char*)data + bytes;
    for (;;) {
        if (s >= e) { break; }
        const char* line = s;
        int32_t k = line_bytes(&s, e);
        if (str.starts_with(line, include_name)) {
            char fn[1024];
            const char* header = line + strlen(include_name);
            strprintf(fn, "%.*s", k - str.length(include_name) - 1, header);
            if (!includes_contain(fn)) {
                char inc_file[1024];
                strprintf(inc_file, "inc/%s/%s", name, fn);
                includes_add(fn);
                cat_header_file(inc_file);
            }
        } else {
            printf("%.*s\n", k, line);
        }
    }
    mem.unmap(data, bytes);
    printf("\n");
}


static void process_src(const char* src) {
    printf("\n");
    printf("#ifndef %s_implementation\n", name);
    printf("#define %s_implementation\n", name);
    printf("\n");
    printf("begin_c\n");
    printf("\n");
    folder_t* f = null;
    fatal_if(folders.open(&f, src) != 0);
    int32_t count = folders.count(f);
    for (int32_t i = 0; i < count; i++) {
        const char* fn = folders.name(f, i);
        header(fn);
        char src_file[1024];
        strprintf(src_file, "src/%s/%s", name, fn);
        cat_source_file(src_file);
    }
    folders.close(f);
    printf("\n");
    printf("end_c\n");
    printf("\n");
    printf("#endif // %s_implementation\n", name);
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
    traceln("%s", folder);
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
    char root[files_max_path];
    strprintf(root, "%s/%s.h", inc, name);
    if (!files.exists(root)) {
        fprintf(stderr, "file not found: \"%s\"", root);
        runtime.exit(usage());
    }
    strprintf(include_name, "#include \"%s/", name);
    process_root(root);
    process_src(src);
    return 0;
}
