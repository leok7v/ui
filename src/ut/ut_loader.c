#include "ut/ut.h"
#include "ut/ut_win32.h"

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

static void* ut_loader_all;

static void* ut_loader_sym_all(const char* name) {
    void* sym = null;
    DWORD bytes = 0;
    ut_fatal_win32err(EnumProcessModules(GetCurrentProcess(),
                                         null, 0, &bytes));
    ut_assert(bytes % sizeof(HMODULE) == 0);
    ut_assert(bytes / sizeof(HMODULE) < 1024); // OK to allocate 8KB on stack
    HMODULE* modules = null;
    ut_fatal_if_error(ut_heap.allocate(null, (void**)&modules, bytes, false));
    ut_fatal_win32err(EnumProcessModules(GetCurrentProcess(),
                                         modules, bytes, &bytes));
    const int32_t n = bytes / (int32_t)sizeof(HMODULE);
    for (int32_t i = 0; i < n && sym != null; i++) {
        sym = ut_loader.sym(modules[i], name);
    }
    if (sym == null) {
        sym = ut_loader.sym(GetModuleHandleA(null), name);
    }
    ut_heap.deallocate(null, modules);
    return sym;
}

static void* ut_loader_open(const char* filename, int32_t ut_unused(mode)) {
    return filename == null ? &ut_loader_all : (void*)LoadLibraryA(filename);
}

static void* ut_loader_sym(void* handle, const char* name) {
    return handle == &ut_loader_all ?
            (void*)ut_loader_sym_all(name) :
            (void*)GetProcAddress((HMODULE)handle, name);
}

static void ut_loader_close(void* handle) {
    if (handle != &ut_loader_all) {
        ut_fatal_win32err(FreeLibrary(handle));
    }
}

#ifdef UT_TESTS

static int32_t ut_loader_test_count;

ut_export void ut_loader_test_exported_function(void);

void ut_loader_test_exported_function(void) { ut_loader_test_count++; }

static void ut_loader_test(void) {
    ut_loader_test_count = 0;
    ut_loader_test_exported_function(); // to make sure it is linked in
    ut_swear(ut_loader_test_count == 1);
    void* global = ut_loader.open(null, ut_loader.local);
    typedef void (*foo_t)(void);
    foo_t foo = (foo_t)ut_loader.sym(global, "ut_loader_test_exported_function");
    foo();
    ut_swear(ut_loader_test_count == 2);
    ut_loader.close(global);
    // NtQueryTimerResolution - http://undocumented.ntinternals.net/
    typedef long (__stdcall *query_timer_resolution_t)(
        long* minimum_resolution,
        long* maximum_resolution,
        long* current_resolution);
    void* nt_dll = ut_loader.open("ntdll", ut_loader.local);
    query_timer_resolution_t query_timer_resolution =
        (query_timer_resolution_t)ut_loader.sym(nt_dll, "NtQueryTimerResolution");
    // in 100ns = 0.1us units
    long min_resolution = 0;
    long max_resolution = 0; // lowest possible delay between timer events
    long cur_resolution = 0;
    ut_fatal_if(query_timer_resolution(
        &min_resolution, &max_resolution, &cur_resolution) != 0);
//  if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
//      ut_println("timer resolution min: %.3f max: %.3f cur: %.3f millisecond",
//          min_resolution / 10.0 / 1000.0,
//          max_resolution / 10.0 / 1000.0,
//          cur_resolution / 10.0 / 1000.0);
//      // Interesting observation cur_resolution sometimes 15.625ms or 1.0ms
//  }
    ut_loader.close(nt_dll);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_println("done"); }
}

#else

static void ut_loader_test(void) {}

#endif

enum {
    ut_loader_local  = 0,       // RTLD_LOCAL  All symbols are not made available for relocation processing by other modules.
    ut_loader_lazy   = 1,       // RTLD_LAZY   Relocations are performed at an implementation-dependent time.
    ut_loader_now    = 2,       // RTLD_NOW    Relocations are performed when the object is loaded.
    ut_loader_global = 0x00100, // RTLD_GLOBAL All symbols are available for relocation processing of other modules.
};

ut_loader_if ut_loader = {
    .local  = ut_loader_local,
    .lazy   = ut_loader_lazy,
    .now    = ut_loader_now,
    .global = ut_loader_global,
    .open   = ut_loader_open,
    .sym    = ut_loader_sym,
    .close  = ut_loader_close,
    .test   = ut_loader_test
};
