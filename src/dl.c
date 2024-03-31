#include "rt.h"
#include "win32.h"

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

static void* dl_open(const char* filename, int unused(mode)) {
    return (void*)LoadLibraryA(filename);
}

static void* dl_sym(void* handle, const char* name) {
    return (void*)GetProcAddress((HMODULE)handle, name);
}

static void dl_close(void* handle) {
    fatal_if_false(FreeLibrary(handle));
}

static void dl_test(int32_t verbosity) {
    // TODO: implement me
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

