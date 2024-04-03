#include "runtime.h"
#include "win32.h"

static errno_t mem_map_view_of_file(HANDLE file,
        void* *data, int64_t *bytes, bool rw) {
    errno_t r = 0;
    void* address = null;
    HANDLE mapping = CreateFileMapping(file, null,
        rw ? PAGE_READWRITE : PAGE_READONLY,
        (uint32_t)(*bytes >> 32), (uint32_t)*bytes, null);
    if (mapping == null) {
        r = runtime.err();
    } else {
        DWORD access = rw ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
        address = MapViewOfFile(mapping, access, 0, 0, *bytes);
        if (address == null) { r = runtime.err(); }
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

// see: https://learn.microsoft.com/en-us/windows/win32/secauthz/enabling-and-disabling-privileges-in-c--

static errno_t mem_set_token_privilege(void* token,
            const char* name, bool e) {
    TOKEN_PRIVILEGES tp = { .PrivilegeCount = 1 };
    tp.Privileges[0].Attributes = e ? SE_PRIVILEGE_ENABLED : 0;
    fatal_if_false(LookupPrivilegeValueA(null, name, &tp.Privileges[0].Luid));
    return b2e(AdjustTokenPrivileges(token, false, &tp,
               sizeof(TOKEN_PRIVILEGES), null, null));
}

static errno_t mem_adjust_process_privilege_manage_volume_name(void) {
    // see: https://devblogs.microsoft.com/oldnewthing/20160603-00/?p=93565
    const uint32_t access = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
    const HANDLE process = GetCurrentProcess();
    HANDLE token = null;
    errno_t r = b2e(OpenProcessToken(process, access, &token));
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

static errno_t mem_map_file(const char* filename, void* *data,
        int64_t *bytes, bool rw) {
    if (rw) { // for SetFileValidData() call:
        (void)mem_adjust_process_privilege_manage_volume_name();
    }
    errno_t r = 0;
    const DWORD access = GENERIC_READ | (rw ? GENERIC_WRITE : 0);
    const DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const DWORD disposition = rw ? OPEN_ALWAYS : OPEN_EXISTING;
    const DWORD flags = FILE_ATTRIBUTE_NORMAL;
    HANDLE file = CreateFileA(filename, access, share, null, disposition,
                              flags, null);
    if (file == INVALID_HANDLE_VALUE) {
        r = runtime.err();
    } else {
        LARGE_INTEGER eof = { .QuadPart = 0 };
        fatal_if_false(GetFileSizeEx(file, &eof));
        if (rw && *bytes > eof.QuadPart) { // increase file size
            const LARGE_INTEGER size = { .QuadPart = *bytes };
            r = r != 0 ? r : (b2e(SetFilePointerEx(file, size, null, FILE_BEGIN)));
            r = r != 0 ? r : (b2e(SetEndOfFile(file)));
            // the following not guaranteed to work but helps with sparse files
            r = r != 0 ? r : (b2e(SetFileValidData(file, *bytes)));
            // SetFileValidData() only works for Admin (verified) or System accounts
            if (r == ERROR_PRIVILEGE_NOT_HELD) { r = 0; } // ignore
            // SetFileValidData() is also semi-security hole because it allows to read
            // previously not zeroed disk content of other files
            const LARGE_INTEGER zero = { .QuadPart = 0 }; // rewind stream:
            r = r != 0 ? r : (b2e(SetFilePointerEx(file, zero, null, FILE_BEGIN)));
        } else {
            *bytes = eof.QuadPart;
        }
        r = r != 0 ? r : mem_map_view_of_file(file, data, bytes, rw);
        fatal_if_false(CloseHandle(file));
    }
    return r;
}

static errno_t mem_map_ro(const char* filename, void* *data, int64_t *bytes) {
    return mem_map_file(filename, data, bytes, false);
}

static errno_t mem_map_rw(const char* filename, void* *data, int64_t *bytes) {
    return mem_map_file(filename, data, bytes, true);
}

static void mem_unmap(void* data, int64_t bytes) {
    assert(data != null && bytes > 0);
    (void)bytes; /* unused only need for posix version */
    if (data != null && bytes > 0) {
        fatal_if_false(UnmapViewOfFile(data));
    }
}

static errno_t mem_map_resource(const char* label, void* *data, int64_t *bytes) {
    HRSRC res = FindResourceA(null, label, (const char*)RT_RCDATA);
    // "LockResource does not actually lock memory; it is just used to
    // obtain a pointer to the memory containing the resource data.
    // The name of the function comes from versions prior to Windows XP,
    // when it was used to lock a global memory block allocated by LoadResource."
    if (res != null) { *bytes = SizeofResource(null, res); }
    HGLOBAL g = res != null ? LoadResource(null, res) : null;
    *data = g != null ? LockResource(g) : null;
    return *data != null ? 0 : runtime.err();
}

static int32_t mem_page_size(void) {
    static SYSTEM_INFO system_info;
    if (system_info.dwPageSize == 0) {
        GetSystemInfo(&system_info);
    }
    return (int32_t)system_info.dwPageSize;
}

static int mem_large_page_size(void) {
    static SIZE_T large_page_minimum = 0;
    if (large_page_minimum == 0) {
        large_page_minimum = GetLargePageMinimum();
    }
    return (int32_t)large_page_minimum;
}

static void* mem_alloc_pages(int64_t bytes_multiple_of_page_size) {
    assert(bytes_multiple_of_page_size > 0);
    SIZE_T bytes = (SIZE_T)bytes_multiple_of_page_size;
    int page_size = mem_page_size();
    assert(bytes % page_size == 0);
    int r = 0;
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
            r = runtime.err();
            if (r != 0) {
                traceln("VirtualAlloc(%lld) failed %s", bytes, str.error(r));
            }
        } else {
            r = VirtualLock(a, bytes) ? 0 : runtime.err();
            if (r == ERROR_WORKING_SET_QUOTA) {
                // The default size is 345 pages (for example,
                // this is 1,413,120 bytes on systems with a 4K page size).
                SIZE_T min_mem = 0, max_mem = 0;
                r = b2e(GetProcessWorkingSetSize(GetCurrentProcess(), &min_mem, &max_mem));
                if (r != 0) {
                    traceln("GetProcessWorkingSetSize() failed %s", str.error(r));
                } else {
                    max_mem =  max_mem + bytes * 2LL;
                    max_mem = (max_mem + page_size - 1) / page_size * page_size +
                               page_size * 16;
                    if (min_mem < max_mem) { min_mem = max_mem; }
                    r = b2e(SetProcessWorkingSetSize(GetCurrentProcess(),
                            min_mem, max_mem));
                    if (r != 0) {
                        traceln("SetProcessWorkingSetSize(%lld, %lld) failed %s",
                            (uint64_t)min_mem, (uint64_t)max_mem, str.error(r));
                    } else {
                        r = b2e(VirtualLock(a, bytes));
                    }
                }
            }
            if (r != 0) {
                traceln("VirtualLock(%lld) failed %s", bytes, str.error(r));
            }
        }
    }
    if (r != 0) {
        traceln("mem_alloc_pages(%lld) failed %s", bytes, str.error(r));
        assert(a == null);
    }
    return a;
}

static void mem_free_pages(void* a, int64_t bytes_multiple_of_page_size) {
    assert(bytes_multiple_of_page_size > 0);
    SIZE_T bytes = (SIZE_T)bytes_multiple_of_page_size;
    int r = 0;
    int page_size = mem_page_size();
    if (bytes_multiple_of_page_size < 0 || bytes % page_size != 0) {
        r = EINVAL;
        traceln("failed %s", str.error(r));
    } else {
        if (a != null) {
            // in case it was successfully locked
            r = b2e(VirtualUnlock(a, bytes));
            if (r != 0) {
                traceln("VirtualUnlock() failed %s", str.error(r));
            }
            // If the "dwFreeType" parameter is MEM_RELEASE, "dwSize" parameter
            // must be the base address returned by the VirtualAlloc function when
            // the region of pages is reserved.
            r = b2e(VirtualFree(a, 0, MEM_RELEASE));
            if (r != 0) { traceln("VirtuaFree() failed %s", str.error(r)); }
        }
    }
}

static heap_t* mem_heap_create(bool serialized) {
    const DWORD options = serialized ? 0 : HEAP_NO_SERIALIZE;
    return (heap_t*)HeapCreate(options, 0, 0);
}

static void mem_heap_dispose(heap_t* heap) {
    fatal_if_false(HeapDestroy((HANDLE)heap));
}

static inline HANDLE mem_heap(heap_t* heap) {
    static HANDLE process_heap;
    if (process_heap == null) { process_heap = GetProcessHeap(); }
    return heap != null ? (HANDLE)heap : process_heap;
}

static void* mem_heap_allocate(heap_t* heap, int64_t bytes, bool zero) {
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    return HeapAlloc(mem_heap(heap), flags, (SIZE_T)bytes);
}

static void* mem_heap_reallocate(heap_t* heap, void* a, int64_t bytes, bool zero) {
    const DWORD flags = zero ? HEAP_ZERO_MEMORY : 0;
    return HeapReAlloc(mem_heap(heap), flags, a, (SIZE_T)bytes);
}

static void mem_heap_deallocate(heap_t* heap, void* a) {
    fatal_if_false(HeapFree(mem_heap(heap), 0, a));
}

static int64_t mem_heap_bytes(heap_t* heap, void* a) {
    SIZE_T bytes = HeapSize(mem_heap(heap), 0, a);
    fatal_if(bytes == (SIZE_T)-1);
    return (int64_t)bytes;
}

static void mem_test(void) {
    #ifdef RUNTIME_TESTS
    swear(args.c > 0);
    void* data = null;
    int64_t bytes = 0;
    swear(mem.map_ro(args.v[0], &data, &bytes) == 0);
    swear(data != null && bytes != 0);
    mem.unmap(data, bytes);
    // TODO: page_size large_page_size alloc_pages free_pages
    // TODO: test heap functions
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #endif
}

mem_if mem = {
    .heap = {
        .create      = mem_heap_create,
        .allocate    = mem_heap_allocate,
        .reallocate  = mem_heap_reallocate,
        .deallocate  = mem_heap_deallocate,
        .bytes       = mem_heap_bytes,
        .dispose     = mem_heap_dispose
    },
    .map_ro          = mem_map_ro,
    .map_rw          = mem_map_rw,
    .unmap           = mem_unmap,
    .map_resource    = mem_map_resource,
    .page_size       = mem_page_size,
    .large_page_size = mem_large_page_size,
    .alloc_pages     = mem_alloc_pages,
    .free_pages      = mem_free_pages,
    .test            = mem_test
};


