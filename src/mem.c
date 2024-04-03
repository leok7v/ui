#include "runtime.h"
#include "win32.h"

static int mem_map_view_of_file(HANDLE file, void* *data, int64_t *bytes,
        bool rw) {
    errno_t r = 0;
    void* address = null;
    HANDLE mapping = CreateFileMapping(file, null,
        rw ? PAGE_READWRITE : PAGE_READONLY,
        (uint32_t)(*bytes >> 32), (uint32_t)*bytes, null);
    if (mapping == null) {
        r = GetLastError();
    } else {
        address = MapViewOfFile(mapping,
            rw ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ,
            0, 0, *bytes);
        if (address == null) { r = GetLastError(); }
        fatal_if_false(CloseHandle(mapping));
    }
    if (r == 0) {
        *data = address;
    } else {
        *data = null;
        *bytes = 0;
    }
    return r;
}

static int mem_set_token_privilege(void* token, const char* name, bool e) {
    // see: https://learn.microsoft.com/en-us/windows/win32/secauthz/enabling-and-disabling-privileges-in-c--
    TOKEN_PRIVILEGES tp = { .PrivilegeCount = 1 };
    tp.Privileges[0].Attributes = e ? SE_PRIVILEGE_ENABLED : 0;
    fatal_if_false(LookupPrivilegeValueA(null, name, &tp.Privileges[0].Luid));
    return AdjustTokenPrivileges(token, false, &tp,
           sizeof(TOKEN_PRIVILEGES), null, null) ? 0 : GetLastError();
}

static int mem_adjust_process_privilege_manage_volume_name(void) {
    // see: https://devblogs.microsoft.com/oldnewthing/20160603-00/?p=93565
    const uint32_t access = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
    const HANDLE process = GetCurrentProcess();
    HANDLE token = null;
    errno_t r = OpenProcessToken(process, access, &token) ? 0 : GetLastError();
    if (r == 0) {
        #ifdef UNICODE
        const char* se_manage_volume_name = utf16to8(SE_MANAGE_VOLUME_NAME);
        #else
        const char* se_manage_volume_name = SE_MANAGE_VOLUME_NAME;
        #endif
        r = mem_set_token_privilege(token, se_manage_volume_name, true);
        fatal_if_false(CloseHandle(token));
    }
    return r;
}

static int mem_map_file(const char* filename, void* *data,
        int64_t *bytes, bool rw) {
    if (rw) { // for SetFileValidData() call:
        (void)mem_adjust_process_privilege_manage_volume_name();
    }
    errno_t r = 0;
    const DWORD flags = GENERIC_READ | (rw ? GENERIC_WRITE : 0);
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    HANDLE file = CreateFileA(filename, flags, share, null,
        rw ? OPEN_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = GetLastError();
    } else {
        LARGE_INTEGER eof = { .QuadPart = 0 };
        fatal_if_false(GetFileSizeEx(file, &eof));
        if (rw && *bytes > eof.QuadPart) { // increase file size
            const LARGE_INTEGER size = { .QuadPart = *bytes };
            r = r != 0 ? r : (SetFilePointerEx(file, size, null, FILE_BEGIN) ? 0 : GetLastError());
            r = r != 0 ? r : (SetEndOfFile(file) ? 0 : GetLastError());
            // the following not guaranteed to work but helps with sparse files
            r = r != 0 ? r : (SetFileValidData(file, *bytes) ? 0 : GetLastError());
            // SetFileValidData() only works for Admin (verified) or System accounts
            if (r == ERROR_PRIVILEGE_NOT_HELD) { r = 0; } // ignore
            // SetFileValidData() is also semi-security hole because it allows to read
            // previously not zeroed disk content of other files
            const LARGE_INTEGER zero = { .QuadPart = 0 }; // rewind stream:
            r = r != 0 ? r : (SetFilePointerEx(file, zero, null, FILE_BEGIN) ? 0 : GetLastError());
        } else {
            *bytes = eof.QuadPart;
        }
        r = r != 0 ? r : mem_map_view_of_file(file, data, bytes, rw);
        fatal_if_false(CloseHandle(file));
    }
    return r;
}

static int mem_map_ro(const char* filename, void* *data, int64_t *bytes) {
    return mem_map_file(filename, data, bytes, false);
}

static int mem_map_rw(const char* filename, void* *data, int64_t *bytes) {
    return mem_map_file(filename, data, bytes, true);
}

static void mem_unmap(void* data, int64_t bytes) {
    assert(data != null && bytes > 0);
    (void)bytes; /* unused only need for posix version */
    if (data != null && bytes > 0) {
        fatal_if_false(UnmapViewOfFile(data));
    }
}

static int mem_map_resource(const char* label, void* *data, int64_t *bytes) {
    HRSRC res = FindResourceA(null, label, (const char*)RT_RCDATA);
    // "LockResource does not actually lock memory; it is just used to
    // obtain a pointer to the memory containing the resource data.
    // The name of the function comes from versions prior to Windows XP,
    // when it was used to lock a global memory block allocated by LoadResource."
    if (res != null) { *bytes = SizeofResource(null, res); }
    HGLOBAL g = res != null ? LoadResource(null, res) : null;
    *data = g != null ? LockResource(g) : null;
    return *data != null ? 0 : GetLastError();
}

static void mem_test(void) {
    #ifdef RUNTIME_TESTS
    swear(args.c > 0);
    void* data = null;
    int64_t bytes = 0;
    swear(mem.map_ro(args.v[0], &data, &bytes) == 0);
    swear(data != null && bytes != 0);
    mem.unmap(data, bytes);
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #endif
}

mem_if mem = {
    .map_ro       = mem_map_ro,
    .map_rw       = mem_map_rw,
    .unmap        = mem_unmap,
    .map_resource = mem_map_resource,
    .test         = mem_test
};


