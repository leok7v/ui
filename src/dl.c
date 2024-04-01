#include "rt.h"
#include "win32.h"
#include <psapi.h>

// This is oversimplified Win32 version completely ignoring mode.

// I bit more Posix compliant version is here:
// https://github.com/dlfcn-win32/dlfcn-win32/blob/master/src/dlfcn.c
// POSIX says that if the value of file is NULL, a handle on a global
// symbol object must be provided. That object must be able to access
// all symbols from the original program file, and any objects loaded
// with the RTLD_GLOBAL flag.
// The return value from GetModuleHandle( ) allows us to retrieve
// symbols only from the original program file. EnumProcessModules() is
// used to access symbols from other libraries. For objects loaded
// with the RTLD_LOCAL flag, we create our own list later on. They are
// excluded from EnumProcessModules() iteration.


static void* dl_all;

static void* dl_sym_all(const char* name) {
    void* sym = null;
    DWORD bytes = 0;
    fatal_if_false(EnumProcessModules(GetCurrentProcess(), null, 0, &bytes));
    assert(bytes % sizeof(HMODULE) == 0);
    assert(bytes / sizeof(HMODULE) < 1024); // OK to allocate 8KB on stack
    HMODULE* modules = stackalloc(bytes);
    fatal_if_false(EnumProcessModules(GetCurrentProcess(),
        modules, bytes, &bytes));
    const int32_t n = bytes / (int)sizeof(HMODULE);
    for (int32_t i = 0; i < n && sym != null; i++) {
        sym = dl.sym(modules[i], name);
    }
    if (sym == null) {
        sym = dl.sym(GetModuleHandleA(null), name);
    }
    return sym;
}

static void* dl_open(const char* filename, int unused(mode)) {
    return filename == null ? &dl_all : (void*)LoadLibraryA(filename);
}

static void* dl_sym(void* handle, const char* name) {
    return handle == &dl_all ?
            (void*)dl_sym_all(name) :
            (void*)GetProcAddress((HMODULE)handle, name);
}

static void dl_close(void* handle) {
    if (handle != &dl_all) {
        fatal_if_false(FreeLibrary(handle));
    }
}

static int32_t dl_test_count;

export void dl_test_exported_function(void) { dl_test_count++; }

static void dl_test(int32_t verbosity) {
    dl_test_count = 0;
    dl_test_exported_function(); // to make sure it is linked in
    swear(dl_test_count == 1);
    void* global = dl.open(null, dl.local);
    typedef void (*foo_t)(void);
    foo_t foo = (foo_t)dl.sym(global, "dl_test_exported_function");
    foo();
    swear(dl_test_count == 2);
    dl.close(global);
    // NtQueryTimerResolution - http://undocumented.ntinternals.net/
    typedef long (__stdcall *nt_query_timer_resolution_t)(
        long* minimum_resolution,
        long* maximum_resolution,
        long* current_resolution);
    void* nt_dll = dl.open("ntdll", dl.local);
    nt_query_timer_resolution_t query_timer_resolution =
        (nt_query_timer_resolution_t)dl.sym(nt_dll, "NtQueryTimerResolution");
    // in 100ns = 0.1us units
    long min_resolution = 0;
    long max_resolution = 0; //  lowest possible delay between timer events
    long cur_resolution = 0;
    fatal_if(query_timer_resolution(
        &min_resolution, &max_resolution, &cur_resolution) != 0);
    if (verbosity > 1) {
        traceln("timer resolution min: %.3f max: %.3f cur: %.3f microsecond",
            min_resolution / 10000.0,
            max_resolution / 10000.0,
            cur_resolution / 10000.0);
    }
    dl.close(nt_dll);
    if (verbosity > 0) { traceln("done"); }
}

enum {
    dl_local  = 0,       // RTLD_LOCAL  All symbols are not made available for relocation processing by other modules.
    dl_lazy   = 1,       // RTLD_LAZY   Relocations are performed at an implementation-dependent time.
    dl_now    = 2,       // RTLD_NOW    Relocations are performed when the object is loaded.
    dl_global = 0x00100, // RTLD_GLOBAL All symbols are available for relocation processing of other modules.
};

dl_if dl = {
    .local  = dl_local,
    .lazy   = dl_lazy,
    .now    = dl_now,
    .global = dl_global,
    .open   = dl_open,
    .sym    = dl_sym,
    .close  = dl_close,
    .test   = dl_test
};

end_c

