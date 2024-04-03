#include "runtime.h"
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


static void* loader_all;

static void* loader_sym_all(const char* name) {
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
        sym = loader.sym(modules[i], name);
    }
    if (sym == null) {
        sym = loader.sym(GetModuleHandleA(null), name);
    }
    return sym;
}

static void* loader_open(const char* filename, int unused(mode)) {
    return filename == null ? &loader_all : (void*)LoadLibraryA(filename);
}

static void* loader_sym(void* handle, const char* name) {
    return handle == &loader_all ?
            (void*)loader_sym_all(name) :
            (void*)GetProcAddress((HMODULE)handle, name);
}

static void loader_close(void* handle) {
    if (handle != &loader_all) {
        fatal_if_false(FreeLibrary(handle));
    }
}

#ifdef RUNTIME_TESTS

static int32_t loader_test_count;

export void loader_test_exported_function(void) { loader_test_count++; }

static void loader_test(void) {
    loader_test_count = 0;
    loader_test_exported_function(); // to make sure it is linked in
    swear(loader_test_count == 1);
    void* global = loader.open(null, loader.local);
    typedef void (*foo_t)(void);
    foo_t foo = (foo_t)loader.sym(global, "loader_test_exported_function");
    foo();
    swear(loader_test_count == 2);
    loader.close(global);
    // NtQueryTimerResolution - http://undocumented.ntinternals.net/
    typedef long (__stdcall *query_timer_resolution_t)(
        long* minimum_resolution,
        long* maximum_resolution,
        long* current_resolution);
    void* nt_dll = loader.open("ntdll", loader.local);
    query_timer_resolution_t query_timer_resolution =
        (query_timer_resolution_t)loader.sym(nt_dll, "NtQueryTimerResolution");
    // in 100ns = 0.1us units
    long min_resolution = 0;
    long max_resolution = 0; // lowest possible delay between timer events
    long cur_resolution = 0;
    fatal_if(query_timer_resolution(
        &min_resolution, &max_resolution, &cur_resolution) != 0);
    if (debug.verbosity.level >= debug.verbosity.trace) {
        traceln("timer resolution min: %.3f max: %.3f cur: %.3f millisecond",
            min_resolution / 10.0 / 1000.0,
            max_resolution / 10.0 / 1000.0,
            cur_resolution / 10.0 / 1000.0);
        // Interesting observation cur_resolution sometimes 15.625ms or 1.0ms
    }
    loader.close(nt_dll);
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
}

#else

static void loader_test(void) {}

#endif


enum {
    loader_local  = 0,       // RTLD_LOCAL  All symbols are not made available for relocation processing by other modules.
    loader_lazy   = 1,       // RTLD_LAZY   Relocations are performed at an implementation-dependent time.
    loader_now    = 2,       // RTLD_NOW    Relocations are performed when the object is loaded.
    loader_global = 0x00100, // RTLD_GLOBAL All symbols are available for relocation processing of other modules.
};

loader_if loader = {
    .local  = loader_local,
    .lazy   = loader_lazy,
    .now    = loader_now,
    .global = loader_global,
    .open   = loader_open,
    .sym    = loader_sym,
    .close  = loader_close,
    .test   = loader_test
};

end_c

