#include "rt/rt.h"
#include "rt/rt_win32.h"

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

static void* rt_loader_all;

static void* rt_loader_sym_all(const char* name) {
    void* sym = null;
    DWORD bytes = 0;
    rt_fatal_win32err(EnumProcessModules(GetCurrentProcess(),
                                         null, 0, &bytes));
    rt_assert(bytes % sizeof(HMODULE) == 0);
    rt_assert(bytes / sizeof(HMODULE) < 1024); // OK to allocate 8KB on stack
    HMODULE* modules = null;
    rt_fatal_if_error(rt_heap.allocate(null, (void**)&modules, bytes, false));
    rt_fatal_win32err(EnumProcessModules(GetCurrentProcess(),
                                         modules, bytes, &bytes));
    const int32_t n = bytes / (int32_t)sizeof(HMODULE);
    for (int32_t i = 0; i < n && sym != null; i++) {
        sym = rt_loader.sym(modules[i], name);
    }
    if (sym == null) {
        sym = rt_loader.sym(GetModuleHandleA(null), name);
    }
    rt_heap.deallocate(null, modules);
    return sym;
}

static void* rt_loader_open(const char* filename, int32_t rt_unused(mode)) {
    return filename == null ? &rt_loader_all : (void*)LoadLibraryA(filename);
}

static void* rt_loader_sym(void* handle, const char* name) {
    return handle == &rt_loader_all ?
            (void*)rt_loader_sym_all(name) :
            (void*)GetProcAddress((HMODULE)handle, name);
}

static void rt_loader_close(void* handle) {
    if (handle != &rt_loader_all) {
        rt_fatal_win32err(FreeLibrary(handle));
    }
}

#ifdef RT_TESTS

#define RT_LOADER_TEST_EXPORTED_FUNCTION

#ifdef RT_LOADER_TEST_EXPORTED_FUNCTION

// manually test once and comment out because creating .lib
// out of each .exe is annoying

static int32_t rt_loader_test_calls_count;

rt_export void rt_loader_test_exported_function(void);

void rt_loader_test_exported_function(void) { rt_loader_test_calls_count++; }

#endif

static void rt_loader_test(void) {
    void* global = rt_loader.open(null, rt_loader.local);
    rt_loader.close(global);
    // NtQueryTimerResolution - http://undocumented.ntinternals.net/
    typedef long (__stdcall *query_timer_resolution_t)(
        long* minimum_resolution,
        long* maximum_resolution,
        long* current_resolution);
    void* nt_dll = rt_loader.open("ntdll", rt_loader.local);
    query_timer_resolution_t query_timer_resolution =
        (query_timer_resolution_t)rt_loader.sym(nt_dll, "NtQueryTimerResolution");
    // in 100ns = 0.1us units
    long min_resolution = 0;
    long max_resolution = 0; // lowest possible delay between timer events
    long cur_resolution = 0;
    rt_fatal_if(query_timer_resolution(
        &min_resolution, &max_resolution, &cur_resolution) != 0);
//  if (rt_debug.verbosity.level >= rt_debug.verbosity.trace) {
//      rt_println("timer resolution min: %.3f max: %.3f cur: %.3f millisecond",
//          min_resolution / 10.0 / 1000.0,
//          max_resolution / 10.0 / 1000.0,
//          cur_resolution / 10.0 / 1000.0);
//      // Interesting observation cur_resolution sometimes 15.625ms or 1.0ms
//  }
    rt_loader.close(nt_dll);
#ifdef RT_LOADER_TEST_EXPORTED_FUNCTION
    rt_loader_test_calls_count = 0;
    rt_loader_test_exported_function(); // to make sure it is linked in
    rt_swear(rt_loader_test_calls_count == 1);
    typedef void (*foo_t)(void);
    foo_t foo = (foo_t)rt_loader.sym(global, "rt_loader_test_exported_function");
    foo();
    rt_swear(rt_loader_test_calls_count == 2);
#endif
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#else

static void rt_loader_test(void) {}

#endif

enum {
    rt_loader_local  = 0,       // RTLD_LOCAL  All symbols are not made available for relocation processing by other modules.
    rt_loader_lazy   = 1,       // RTLD_LAZY   Relocations are performed at an implementation-dependent time.
    rt_loader_now    = 2,       // RTLD_NOW    Relocations are performed when the object is loaded.
    rt_loader_global = 0x00100, // RTLD_GLOBAL All symbols are available for relocation processing of other modules.
};

rt_loader_if rt_loader = {
    .local  = rt_loader_local,
    .lazy   = rt_loader_lazy,
    .now    = rt_loader_now,
    .global = rt_loader_global,
    .open   = rt_loader_open,
    .sym    = rt_loader_sym,
    .close  = rt_loader_close,
    .test   = rt_loader_test
};
