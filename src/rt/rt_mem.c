#include "rt/rt.h"
#include "rt/rt_win32.h"

static errno_t rt_mem_map_view_of_file(HANDLE file,
        void* *data, int64_t *bytes, bool rw) {
    errno_t r = 0;
    void* address = null;
    HANDLE mapping = CreateFileMapping(file, null,
        rw ? PAGE_READWRITE : PAGE_READONLY,
        (uint32_t)(*bytes >> 32), (uint32_t)*bytes, null);
    if (mapping == null) {
        r = rt_core.err();
    } else {
        DWORD access = rw ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
        address = MapViewOfFile(mapping, access, 0, 0, (SIZE_T)*bytes);
        if (address == null) { r = rt_core.err(); }
        rt_win32_close_handle(mapping);
    }
    if (r == 0) {
        *data = address;
    } else {
        *data = null;
        *bytes = 0;
    }
    return r;
}

// see: https://learn.microsoft.com/en-us/windows/win32/secauthz/enabling-and-disabling-privileges-in-c--

static errno_t rt_mem_set_token_privilege(void* token,
            const char* name, bool e) {
    TOKEN_PRIVILEGES tp = { .PrivilegeCount = 1 };
    tp.Privileges[0].Attributes = e ? SE_PRIVILEGE_ENABLED : 0;
    rt_fatal_win32err(LookupPrivilegeValueA(null, name, &tp.Privileges[0].Luid));
    return rt_b2e(AdjustTokenPrivileges(token, false, &tp,
               sizeof(TOKEN_PRIVILEGES), null, null));
}

static errno_t rt_mem_adjust_process_privilege_manage_volume_name(void) {
    // see: https://devblogs.microsoft.com/oldnewthing/20160603-00/?p=93565
    const uint32_t access = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
    const HANDLE process = GetCurrentProcess();
    HANDLE token = null;
    errno_t r = rt_b2e(OpenProcessToken(process, access, &token));
    if (r == 0) {
        const char* se_manage_volume_name = "SeManageVolumePrivilege";
        r = rt_mem_set_token_privilege(token, se_manage_volume_name, true);
        rt_win32_close_handle(token);
    }
    return r;
}

static errno_t rt_mem_map_file(const char* filename, void* *data,
        int64_t *bytes, bool rw) {
    if (rw) { // for SetFileValidData() call:
        (void)rt_mem_adjust_process_privilege_manage_volume_name();
    }
    errno_t r = 0;
    const DWORD access = GENERIC_READ | (rw ? GENERIC_WRITE : 0);
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const DWORD disposition = rw ? OPEN_ALWAYS : OPEN_EXISTING;
    const DWORD flags = FILE_ATTRIBUTE_NORMAL;
    HANDLE file = CreateFileA(filename, access, share, null, disposition,
                              flags, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = rt_core.err();
    } else {
        LARGE_INTEGER eof = { .QuadPart = 0 };
        rt_fatal_win32err(GetFileSizeEx(file, &eof));
        if (rw && *bytes > eof.QuadPart) { // increase file size
            const LARGE_INTEGER size = { .QuadPart = *bytes };
            r = r != 0 ? r : (rt_b2e(SetFilePointerEx(file, size, null, FILE_BEGIN)));
            r = r != 0 ? r : (rt_b2e(SetEndOfFile(file)));
            // the following not guaranteed to work but helps with sparse files
            r = r != 0 ? r : (rt_b2e(SetFileValidData(file, *bytes)));
            // SetFileValidData() only works for Admin (verified) or System accounts
            if (r == ERROR_PRIVILEGE_NOT_HELD) { r = 0; } // ignore
            // SetFileValidData() is also semi-security hole because it allows to read
            // previously not zeroed disk content of other files
            const LARGE_INTEGER zero = { .QuadPart = 0 }; // rewind stream:
            r = r != 0 ? r : (rt_b2e(SetFilePointerEx(file, zero, null, FILE_BEGIN)));
        } else {
            *bytes = eof.QuadPart;
        }
        r = r != 0 ? r : rt_mem_map_view_of_file(file, data, bytes, rw);
        rt_win32_close_handle(file);
    }
    return r;
}

static errno_t rt_mem_map_ro(const char* filename, void* *data, int64_t *bytes) {
    return rt_mem_map_file(filename, data, bytes, false);
}

static errno_t rt_mem_map_rw(const char* filename, void* *data, int64_t *bytes) {
    return rt_mem_map_file(filename, data, bytes, true);
}

static void rt_mem_unmap(void* data, int64_t bytes) {
    rt_assert(data != null && bytes > 0);
    (void)bytes; /* unused only need for posix version */
    if (data != null && bytes > 0) {
        rt_fatal_win32err(UnmapViewOfFile(data));
    }
}

static errno_t rt_mem_map_resource(const char* label, void* *data, int64_t *bytes) {
    HRSRC res = FindResourceA(null, label, (const char*)RT_RCDATA);
    // "LockResource does not actually lock memory; it is just used to
    // obtain a pointer to the memory containing the resource data.
    // The name of the function comes from versions prior to Windows XP,
    // when it was used to lock a global memory block allocated by LoadResource."
    if (res != null) { *bytes = SizeofResource(null, res); }
    HGLOBAL g = res != null ? LoadResource(null, res) : null;
    *data = g != null ? LockResource(g) : null;
    return *data != null ? 0 : rt_core.err();
}

static int32_t rt_mem_page_size(void) {
    static SYSTEM_INFO system_info;
    if (system_info.dwPageSize == 0) {
        GetSystemInfo(&system_info);
    }
    return (int32_t)system_info.dwPageSize;
}

static int rt_mem_large_page_size(void) {
    static SIZE_T large_page_minimum = 0;
    if (large_page_minimum == 0) {
        large_page_minimum = GetLargePageMinimum();
    }
    return (int32_t)large_page_minimum;
}

static void* rt_mem_allocate(int64_t bytes_multiple_of_page_size) {
    rt_assert(bytes_multiple_of_page_size > 0);
    SIZE_T bytes = (SIZE_T)bytes_multiple_of_page_size;
    SIZE_T page_size = (SIZE_T)rt_mem_page_size();
    rt_assert(bytes % page_size == 0);
    errno_t r = 0;
    void* a = null;
    if (bytes_multiple_of_page_size < 0 || bytes % page_size != 0) {
        SetLastError(ERROR_INVALID_PARAMETER);
        r = EINVAL;
    } else {
        const DWORD type = MEM_COMMIT | MEM_RESERVE;
        const DWORD physical = type | MEM_PHYSICAL;
        a = VirtualAlloc(null, bytes, physical, PAGE_READWRITE);
        if (a == null) {
            a = VirtualAlloc(null, bytes, type, PAGE_READWRITE);
        }
        if (a == null) {
            r = rt_core.err();
            if (r != 0) {
                rt_println("VirtualAlloc(%lld) failed %s", bytes, rt_strerr(r));
            }
        } else {
            r = VirtualLock(a, bytes) ? 0 : rt_core.err();
            if (r == ERROR_WORKING_SET_QUOTA) {
                // The default size is 345 pages (for example,
                // this is 1,413,120 bytes on systems with a 4K page size).
                SIZE_T min_mem = 0, max_mem = 0;
                r = rt_b2e(GetProcessWorkingSetSize(GetCurrentProcess(), &min_mem, &max_mem));
                if (r != 0) {
                    rt_println("GetProcessWorkingSetSize() failed %s", rt_strerr(r));
                } else {
                    max_mem =  max_mem + bytes * 2LL;
                    max_mem = (max_mem + page_size - 1) / page_size * page_size +
                               page_size * 16;
                    if (min_mem < max_mem) { min_mem = max_mem; }
                    r = rt_b2e(SetProcessWorkingSetSize(GetCurrentProcess(),
                            min_mem, max_mem));
                    if (r != 0) {
                        rt_println("SetProcessWorkingSetSize(%lld, %lld) failed %s",
                            (uint64_t)min_mem, (uint64_t)max_mem, rt_strerr(r));
                    } else {
                        r = rt_b2e(VirtualLock(a, bytes));
                    }
                }
            }
            if (r != 0) {
                rt_println("VirtualLock(%lld) failed %s", bytes, rt_strerr(r));
            }
        }
    }
    if (r != 0) {
        rt_println("mem_alloc_pages(%lld) failed %s", bytes, rt_strerr(r));
        rt_assert(a == null);
    }
    return a;
}

static void rt_mem_deallocate(void* a, int64_t bytes_multiple_of_page_size) {
    rt_assert(bytes_multiple_of_page_size > 0);
    SIZE_T bytes = (SIZE_T)bytes_multiple_of_page_size;
    errno_t r = 0;
    SIZE_T page_size = (SIZE_T)rt_mem_page_size();
    if (bytes_multiple_of_page_size < 0 || bytes % page_size != 0) {
        r = EINVAL;
        rt_println("failed %s", rt_strerr(r));
    } else {
        if (a != null) {
            // in case it was successfully locked
            r = rt_b2e(VirtualUnlock(a, bytes));
            if (r != 0) {
                rt_println("VirtualUnlock() failed %s", rt_strerr(r));
            }
            // If the "dwFreeType" parameter is MEM_RELEASE, "dwSize" parameter
            // must be the base address returned by the VirtualAlloc function when
            // the region of pages is reserved.
            r = rt_b2e(VirtualFree(a, 0, MEM_RELEASE));
            if (r != 0) { rt_println("VirtuaFree() failed %s", rt_strerr(r)); }
        }
    }
}

static void rt_mem_test(void) {
    #ifdef RT_TESTS
    rt_swear(rt_args.c > 0);
    void* data = null;
    int64_t bytes = 0;
    rt_swear(rt_mem.map_ro(rt_args.v[0], &data, &bytes) == 0);
    rt_swear(data != null && bytes != 0);
    rt_mem.unmap(data, bytes);
    // TODO: page_size large_page_size allocate deallocate
    // TODO: test heap functions
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
    #endif
}

rt_mem_if rt_mem = {
    .map_ro          = rt_mem_map_ro,
    .map_rw          = rt_mem_map_rw,
    .unmap           = rt_mem_unmap,
    .map_resource    = rt_mem_map_resource,
    .page_size       = rt_mem_page_size,
    .large_page_size = rt_mem_large_page_size,
    .allocate        = rt_mem_allocate,
    .deallocate      = rt_mem_deallocate,
    .test            = rt_mem_test
};
