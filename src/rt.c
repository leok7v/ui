#include "rt.h"

#include <immintrin.h>
#include <Windows.h>
#include <immintrin.h> // _tzcnt_u32
// #include <timeapi.h>
// #include <sysinfoapi.h>

begin_c

char* strnchr(const char* s, int n, char ch) {
    while (n > 0 && *s != 0) {
        if (*s == ch) { return (char*)s; }
        s++; n--;
    }
    return null;
}

const char* _strtolc_(char* d, const char* s) {
    char* r = d;
    while (*s != 0) { *d++ = (char)tolower(*s++); }
    *d = 0;
    return r;
}

static void crt_vformat(char* utf8, int count, const char* format, va_list vl) {
    vsnprintf(utf8, count, format, vl);
    utf8[count - 1] = 0;
}

static void crt_sformat(char* utf8, int count, const char* format, ...) {
    va_list vl;
    va_start(vl, format);
    crt.vformat(utf8, count, format, vl);
    va_end(vl);
}

static const char* crt_error_for_language(int32_t error, LANGID language) {
    static thread_local char text[256];
    const DWORD format = FORMAT_MESSAGE_FROM_SYSTEM|
        FORMAT_MESSAGE_IGNORE_INSERTS;
    wchar_t s[256];
    HRESULT hr = 0 <= error && error <= 0xFFFF ?
        HRESULT_FROM_WIN32(error) : error;
    if (FormatMessageW(format, null, hr, language, s, countof(s) - 1,
            (va_list*)null) > 0) {
        s[countof(s) - 1] = 0;
        // remove trailing '\r\n'
        int k = (int)wcslen(s);
        if (k > 0 && s[k - 1] == '\n') { s[k - 1] = 0; }
        k = (int)wcslen(s);
        if (k > 0 && s[k - 1] == '\r') { s[k - 1] = 0; }
        strprintf(text, "0x%08X(%d) \"%s\"", error, error, utf16to8(s));
    } else {
        strprintf(text, "0x%08X(%d)", error, error);
    }
    return text;
}

static const char* crt_error(int32_t error) {
    const LANGID lang = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
    return crt_error_for_language(error, lang);
}

static const char* crt_error_nls(int32_t error) {
    const LANGID lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    return crt_error_for_language(error, lang);
}

// abort does NOT call atexit() functions and
// does NOT flush streams. Also Win32 runtime
// abort() attempt to show Abort/Retry/Ignore
// MessageBox - thus ExitProcess()

static void crt_abort(void) { ExitProcess(ERROR_FATAL_APP_EXIT); }

static void crt_exit(int32_t exit_code) { exit(exit_code); }

static void* crt_dlopen(const char* filename, int unused(mode)) {
    return (void*)LoadLibraryA(filename);
}

static void* crt_dlsym(void* handle, const char* name) {
    return (void*)GetProcAddress((HMODULE)handle, name);
}

static void crt_dlclose(void* handle) {
    fatal_if_false(FreeLibrary(handle));
}

static void* crt_ntdll(void) {
    static HMODULE ntdll;
    if (ntdll == null) {
        ntdll = crt.dlopen("ntdll.dll", 0);
        not_null(ntdll);
    }
    return ntdll;
}

static void crt_sleep(double seconds) {
    assert(seconds >= 0);
    if (seconds < 0) { seconds = 0; }
    int64_t ns100 = (int64_t)(seconds * 1.0e+7); // in 0.1 us aka 100ns
    typedef int (__stdcall *nt_delay_execution_t)(BOOLEAN Alertable,
        PLARGE_INTEGER DelayInterval);
    static nt_delay_execution_t NtDelayExecution;
    // delay in 100-ns units. negative value means delay relative to current.
    LARGE_INTEGER delay = {0}; // delay in 100-ns units.
    delay.QuadPart = -ns100; // negative value means delay relative to current.
    if (NtDelayExecution == null) {
        void* ntdll = crt_ntdll();
        NtDelayExecution = (nt_delay_execution_t)
            crt.dlsym(ntdll, "NtDelayExecution");
        not_null(NtDelayExecution);
    }
    //  If "alertable" is set, execution can break in a result of NtAlertThread call.
    NtDelayExecution(false, &delay);
}

static uint64_t crt_microseconds_since_epoch(void) { // NOT monotonic
    FILETIME ft; // time in 100ns interval (tenth of microsecond)
    // since 12:00 A.M. January 1, 1601 Coordinated Universal Time (UTC)
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t microseconds =
        (((uint64_t)ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10;
    assert(microseconds > 0);
    return microseconds;
}

static uint64_t crt_localtime(void) {
    TIME_ZONE_INFORMATION tzi; // UTC = local time + bias
    GetTimeZoneInformation(&tzi);
    uint64_t bias = (uint64_t)tzi.Bias * 60LL * 1000 * 1000; // in microseconds
    return crt_microseconds_since_epoch() - bias;
}

static void crt_time_utc(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc) {
    uint64_t time_in_100ns = microseconds * 10;
    FILETIME mst = { (DWORD)(time_in_100ns & 0xFFFFFFFF), (DWORD)(time_in_100ns >> 32) };
    SYSTEMTIME utc;
    FileTimeToSystemTime(&mst, &utc);
    *year = utc.wYear;
    *month = utc.wMonth;
    *day = utc.wDay;
    *hh = utc.wHour;
    *mm = utc.wMinute;
    *ss = utc.wSecond;
    *ms = utc.wMilliseconds;
    *mc = microseconds % 1000;
}

static void crt_time_local(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc) {
    uint64_t time_in_100ns = microseconds * 10;
    FILETIME mst = { (DWORD)(time_in_100ns & 0xFFFFFFFF), (DWORD)(time_in_100ns >> 32) };
    SYSTEMTIME utc;
    FileTimeToSystemTime(&mst, &utc);
    DYNAMIC_TIME_ZONE_INFORMATION tzi;
    GetDynamicTimeZoneInformation(&tzi);
    SYSTEMTIME lt = {0};
    SystemTimeToTzSpecificLocalTimeEx(&tzi, &utc, &lt);
    *year = lt.wYear;
    *month = lt.wMonth;
    *day = lt.wDay;
    *hh = lt.wHour;
    *mm = lt.wMinute;
    *ss = lt.wSecond;
    *ms = lt.wMilliseconds;
    *mc = microseconds % 1000;
}

static double crt_seconds(void) { // since_boot
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static double one_over_freq;
    if (one_over_freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        one_over_freq = 1.0 / frequency.QuadPart;
    }
    return (double)qpc.QuadPart * one_over_freq;
}

#define ctz(x) _tzcnt_u32(x)

static uint32_t crt_gcd(uint32_t u, uint32_t v) {
    uint32_t t = u | v;
    if (u == 0 || v == 0) { return t; }
    int g = ctz(t);
    while (u != 0) {
        u >>= ctz(u);
        v >>= ctz(v);
        if (u >= v) {
            u = (u - v) / 2;
        } else {
            v = (v - u) / 2;
        }
    }
    return v << g;
}

static int64_t crt_nanoseconds(void) {
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static uint32_t freq;
    static uint32_t mul = NSEC_IN_SEC;
    if (freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        assert(frequency.HighPart == 0);
        // known values: 10,000,000 and 3,000,000
        // even 1GHz frequency should fit into 32 bit unsigned
        freq = frequency.LowPart;
        assert(freq != 0);
        uint32_t divider = crt_gcd(NSEC_IN_SEC, freq); // avoiding MulDiv64
        freq /= divider;
        mul /= divider;
    }
    uint64_t ns_mul_freq = (uint64_t)qpc.QuadPart * mul;
    return freq == 1 ? ns_mul_freq : ns_mul_freq / freq;
}

static HKEY crt_get_reg_key(const char* name) {
    char path[MAX_PATH];
    strprintf(path, "Software\\app\\%s", name);
    HKEY key = null;
    if (RegOpenKeyA(HKEY_CURRENT_USER, path, &key) != 0) {
        RegCreateKeyA(HKEY_CURRENT_USER, path, &key);
    }
    not_null(key);
    return key;
}

static void crt_data_save(const char* name,
        const char* key, const void* data, int bytes) {
    HKEY k = crt_get_reg_key(name);
    if (k != null) {
        fatal_if_not_zero(RegSetValueExA(k, key, 0, REG_BINARY,
            (byte*)data, bytes));
        fatal_if_not_zero(RegCloseKey(k));
    }
}

static int crt_data_size(const char* name, const char* key) {
    int bytes = -1;
    HKEY k = crt_get_reg_key(name);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = 0;
        int r = RegQueryValueExA(k, key, null, &type, null, &cb);
        if (r == ERROR_FILE_NOT_FOUND) {
            bytes = 0; // do not report data_size() often used this way
        } else if (r != 0) {
            traceln("%s.RegQueryValueExA(\"%s\") failed %s",
                name, key, crt.error(r));
            bytes = 0; // on any error behave as empty data
        } else {
            bytes = (int)cb;
        }
        fatal_if_not_zero(RegCloseKey(k));
    }
    return bytes;
}

static int crt_data_load(const char* name,
        const char* key, void* data, int bytes) {
    int read = -1;
    HKEY k= crt_get_reg_key(name);
    if (k != null) {
        DWORD type = REG_BINARY;
        DWORD cb = (DWORD)bytes;
        int r = RegQueryValueExA(k, key, null, &type, (byte*)data, &cb);
        if (r == ERROR_MORE_DATA) {
            // returns -1 app.data_size() should be used
        } else if (r != 0) {
            if (r != ERROR_FILE_NOT_FOUND) {
                traceln("%s.RegQueryValueExA(\"%s\") failed %s",
                    name, key, crt.error(r));
            }
            read = 0; // on any error behave as empty data
        } else {
            read = (int)cb;
        }
        fatal_if_not_zero(RegCloseKey(k));
    }
    return read;
}

static int64_t crt_ns2ms(int64_t ns) { return (ns + NSEC_IN_MSEC - 1) / NSEC_IN_MSEC; }

typedef int (*gettimerresolution_t)(ULONG* minimum_resolution,
    ULONG* maximum_resolution, ULONG* actual_resolution);
typedef int (*settimerresolution_t)(ULONG requested_resolution,
    BOOLEAN Set, ULONG* actual_resolution);

static int crt_scheduler_set_timer_resolution(int64_t ns) { // nanoseconds
    const int ns100 = (int)(ns / 100);
    void* ntdll = crt_ntdll();
    gettimerresolution_t NtQueryTimerResolution = (gettimerresolution_t)
        crt.dlsym(ntdll, "NtQueryTimerResolution");
    settimerresolution_t NtSetTimerResolution = (settimerresolution_t)
        crt.dlsym(ntdll, "NtSetTimerResolution");
    // it is resolution not frequency this is why it is in reverse
    // to common sense and what is not on Windows?
    unsigned long min_100ns = 16 * 10 * 1000;
    unsigned long max_100ns = 1 * 10 * 1000;
    unsigned long actual_100ns = 0;
    int r = 0;
    if (NtQueryTimerResolution != null &&
        NtQueryTimerResolution(&min_100ns, &max_100ns, &actual_100ns) == 0) {
        int64_t minimum_ns = min_100ns * 100LL;
        int64_t maximum_ns = max_100ns * 100LL;
//      int64_t actual_ns  = actual_100ns  * 100LL;
        // note that maximum resolution is actually < minimum
#ifdef CRT_FALLBACK_TO_TIMEBEGINPERIOD // requires #include <timeapi.h>
        if (NtSetTimerResolution == null) {
            const int milliseconds = (int)(crt_ns2ms(ns) + 0.5);
            r = (int)maximum_ns <= ns && ns <= (int)minimum_ns ?
                timeBeginPeriod(milliseconds) : ERROR_INVALID_PARAMETER;
        } else {
            r = (int)maximum_ns <= ns && ns <= (int)minimum_ns ?
                NtSetTimerResolution(ns100, true, &actual_100ns) :
                ERROR_INVALID_PARAMETER;
        }
#else
        not_null(NtSetTimerResolution);
        r = (int)maximum_ns <= ns && ns <= (int)minimum_ns ?
            NtSetTimerResolution(ns100, true, &actual_100ns) :
            ERROR_INVALID_PARAMETER;
#endif
        NtQueryTimerResolution(&min_100ns, &max_100ns, &actual_100ns);
    } else {
#ifdef CRT_FALLBACK_TO_TIMEBEGINPERIOD // requires #include <timeapi.h>
        const int milliseconds = (int)(crt_ns2ms(ns) + 0.5);
        r = 1 <= milliseconds && milliseconds <= 16 ?
            timeBeginPeriod(milliseconds) : ERROR_INVALID_PARAMETER;
#else
        fatal_if_null(NtQueryTimerResolution);
#endif
    }
    return r;
}

static void crt_power_throttling_disable_for_process(void) {
    static bool disabled_for_the_process;
    if (!disabled_for_the_process) {
        PROCESS_POWER_THROTTLING_STATE pt = { 0 };
        pt.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        pt.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        pt.StateMask = 0;
        fatal_if_false(SetProcessInformation(GetCurrentProcess(),
            ProcessPowerThrottling, &pt, sizeof(pt)));
        // PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION
        // does not work on Win10. There is no easy way to
        // distinguish Windows 11 from 10 (Microsoft great engineering)
        pt.ControlMask = PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION;
        pt.StateMask = 0;
        // ignore error on Windows 10:
        (void)SetProcessInformation(GetCurrentProcess(),
            ProcessPowerThrottling, &pt, sizeof(pt));
        disabled_for_the_process = true;
    }
}

static void crt_power_throttling_disable_for_thread(HANDLE thread) {
    THREAD_POWER_THROTTLING_STATE pt = { 0 };
    pt.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
    pt.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    pt.StateMask = 0;
    fatal_if_false(SetThreadInformation(thread, ThreadPowerThrottling,
        &pt, sizeof(pt)));
}

static void crt_disable_power_throttling(void) {
    crt_power_throttling_disable_for_process();
    crt_power_throttling_disable_for_thread(GetCurrentThread());
}

static uint64_t crt_next_physical_processor_affinity_mask(void) {
    static volatile int32_t initialized;
    static int32_t init;
    static int next = 1; // next physical core to use
    static int cores = 0; // number of physical processors (cores)
    static uint64_t any;
    static uint64_t affinity[64]; // mask for each physical processor
    bool set_to_true = atomics.compare_exchange_int32(&init, false, true);
    if (set_to_true) {
        // Concept D: 6 cores, 12 logical processors: 27 lpi entries
        static SYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi[64];
        DWORD bytes = 0;
        GetLogicalProcessorInformation(null, &bytes);
        assert(bytes % sizeof(lpi[0]) == 0);
        int n = bytes / sizeof(lpi[0]); // number of lpi entries == 27 on 6 core / 12 logical processors system
        assert(bytes <= sizeof(lpi), "increase lpi[%d]", n);
        fatal_if_false(GetLogicalProcessorInformation(&lpi[0], &bytes));
        for (int i = 0; i < n; i++) {
//          traceln("[%2d] affinity mask 0x%016llX relationship=%d %s", i,
//              lpi[i].ProcessorMask, lpi[i].Relationship,
//              rel2str(lpi[i].Relationship));
            if (lpi[i].Relationship == RelationProcessorCore) {
                assert(cores < countof(affinity), "increase affinity[%d]", cores);
                if (cores < countof(affinity)) {
                    any |= lpi[i].ProcessorMask;
                    affinity[cores] = lpi[i].ProcessorMask;
                    cores++;
                }
            }
        }
        initialized = true;
    } else {
        while (initialized == 0) { crt.sleep(1 / 1024.0); }
        assert(any != 0); // should not ever happen
        if (any == 0) { any = (uint64_t)(-1LL); }
    }
    uint64_t mask = next < cores ? affinity[next] : any;
    assert(mask != 0);
    // assume last physical core is least popular
    if (next < cores) { next++; } // not circular
    return mask;
}

static void threads_realtime(void) {
    fatal_if_false(SetPriorityClass(GetCurrentProcess(),
        REALTIME_PRIORITY_CLASS));
    fatal_if_false(SetThreadPriority(GetCurrentThread(),
        THREAD_PRIORITY_TIME_CRITICAL));
    fatal_if_false(SetThreadPriorityBoost(GetCurrentThread(),
        /* bDisablePriorityBoost = */ false));
    fatal_if_not_zero(
        crt_scheduler_set_timer_resolution(NSEC_IN_MSEC));
    fatal_if_false(SetThreadAffinityMask(GetCurrentThread(),
        crt_next_physical_processor_affinity_mask()));
    crt_disable_power_throttling();
}

static int crt_utf8_bytes(const wchar_t* utf16) {
    int required_bytes_count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        utf16, -1, null, 0, null, null);
    assert(required_bytes_count > 0);
    return required_bytes_count;
}

static int crt_utf16_chars(const char* utf8) {
    int required_wide_chars_count = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, null, 0);
    assert(required_wide_chars_count > 0);
    return required_wide_chars_count;
}

static char* crt_utf16to8(char* s, const wchar_t* utf16) {
    int r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, -1, s,
        crt.utf8_bytes(utf16), null, null);
    if (r == 0) {
        traceln("WideCharToMultiByte() failed %s", crt.error(crt.err()));
    }
    return s;
}

static wchar_t* crt_utf8to16(wchar_t* utf16, const char* s) {
    int r = MultiByteToWideChar(CP_UTF8, 0, s, -1, utf16, crt.utf16_chars(s));
    if (r == 0) {
        traceln("WideCharToMultiByte() failed %d", crt.err());
    }
    return utf16;
}

static void crt_breakpoint(void) { if (IsDebuggerPresent()) { DebugBreak(); } }

static int crt_gettid(void) { return (int)GetCurrentThreadId(); }

static int32_t crt_err(void) { return GetLastError(); }

static void crt_seterr(int32_t err) { SetLastError(err); }

static uint32_t crt_random32(uint32_t* state) {
    // https://gist.github.com/tommyettinger/46a874533244883189143505d203312c
    static thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
    uint32_t z = (*state += 0x6D2B79F5UL);
    z = (z ^ (z >> 15)) * (z | 1UL);
    z ^= z + (z ^ (z >> 7)) * (z | 61UL);
    return z ^ (z >> 14);
}

static uint64_t crt_random64(uint64_t *state) {
    // https://gist.github.com/tommyettinger/e6d3e8816da79b45bfe582384c2fe14a
    static thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
	const uint64_t s = *state;
	const uint64_t z = (s ^ s >> 25) * (*state += 0x6A5D39EAE12657AAULL);
	return z ^ (z >> 22);
}

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function

uint32_t crt_hash32(const char *data, int64_t len) {
    uint32_t hash  = 0x811c9dc5;  // FNV_offset_basis for 32-bit
    uint32_t prime = 0x01000193; // FNV_prime for 32-bit
    if (len > 0) {
        for (int64_t i = 1; i < len; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    }
    return hash;
}

uint64_t crt_hash64(const char *data, int64_t len) {
    uint64_t hash  = 0xcbf29ce484222325; // FNV_offset_basis for 64-bit
    uint64_t prime = 0x100000001b3;      // FNV_prime for 64-bit
    if (len > 0) {
        for (int64_t i = 0; i < len; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    }
    return hash;
}

static int crt_memmap_file(HANDLE file, void* *data, int64_t *bytes, bool rw) {
    int r = 0;
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

static int crt_set_token_privilege(void* token, const char* name, bool e) {
    // see: https://learn.microsoft.com/en-us/windows/win32/secauthz/enabling-and-disabling-privileges-in-c--
    TOKEN_PRIVILEGES tp = { .PrivilegeCount = 1 };
    tp.Privileges[0].Attributes = e ? SE_PRIVILEGE_ENABLED : 0;
    fatal_if_false(LookupPrivilegeValueA(null, name, &tp.Privileges[0].Luid));
    return AdjustTokenPrivileges(token, false, &tp,
           sizeof(TOKEN_PRIVILEGES), null, null) ? 0 : GetLastError();
}

static int crt_adjust_process_privilege_manage_volume_name(void) {
    // see: https://devblogs.microsoft.com/oldnewthing/20160603-00/?p=93565
    const uint32_t access = TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY;
    const HANDLE process = GetCurrentProcess();
    HANDLE token = null;
    int r = OpenProcessToken(process, access, &token) ? 0 : GetLastError();
    if (r == 0) {
        #ifdef UNICODE
        const char* se_manage_volume_name = utf16to8(SE_MANAGE_VOLUME_NAME);
        #else
        const char* se_manage_volume_name = SE_MANAGE_VOLUME_NAME;
        #endif
        r = crt_set_token_privilege(token, se_manage_volume_name, true);
        fatal_if_false(CloseHandle(token));
    }
    return r;
}

static int crt_memmap(const char* filename, void* *data,
        int64_t *bytes, bool rw) {
    if (rw) { // for SetFileValidData() call:
        (void)crt_adjust_process_privilege_manage_volume_name();
    }
    int r = 0;
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
        r = r != 0 ? r : crt_memmap_file(file, data, bytes, rw);
        fatal_if_false(CloseHandle(file));
    }
    return r;
}

static int crt_memmap_read(const char* filename, void* *data, int64_t *bytes) {
    return crt_memmap(filename, data, bytes, false);
}

static int crt_memmap_rw(const char* filename, void* *data, int64_t *bytes) {
    return crt_memmap(filename, data, bytes, true);
}

static void crt_memunmap(void* data, int64_t bytes) {
    assert(data != null && bytes > 0);
    (void)bytes; /* unused only need for posix version */
    if (data != null && bytes > 0) {
        fatal_if_false(UnmapViewOfFile(data));
    }
}

static int crt_memmap_res(const char* label, void* *data, int64_t *bytes) {
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

crt_if crt = {
    .err = crt_err,
    .seterr = crt_seterr,
    .abort = crt_abort,
    .exit = crt_exit,
    .dlopen = crt_dlopen,
    .dlsym = crt_dlsym,
    .dlclose = crt_dlclose,
    .random32 = crt_random32,
    .random64 = crt_random64,
    .hash32 = crt_hash32,
    .hash64 = crt_hash64,
    .memmap_read = crt_memmap_read,
    .memmap_rw = crt_memmap_rw,
    .memunmap = crt_memunmap,
    .memmap_res = crt_memmap_res,
    .sleep = crt_sleep,
    .seconds = crt_seconds,
    .nanoseconds = crt_nanoseconds,
    .microseconds = crt_microseconds_since_epoch,
    .localtime = crt_localtime,
    .time_utc = crt_time_utc,
    .time_local = crt_time_local,
    .data_save = crt_data_save,
    .data_size = crt_data_size,
    .data_load = crt_data_load,
    .vformat = crt_vformat,
    .sformat = crt_sformat,
    .error = crt_error,
    .error_nls = crt_error_nls,
    .utf8_bytes = crt_utf8_bytes,
    .utf16_chars = crt_utf16_chars,
    .utf16_utf8 = crt_utf16to8,
    .utf8_utf16 = crt_utf8to16,
    .breakpoint = crt_breakpoint,
    .gettid = crt_gettid
};

#pragma comment(lib, "advapi32")
#pragma comment(lib, "cabinet")
#pragma comment(lib, "comctl32")
#pragma comment(lib, "comdlg32")
#pragma comment(lib, "cfgmgr32")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxva2")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "glu32")
#pragma comment(lib, "imm32")
#pragma comment(lib, "msimg32")
#pragma comment(lib, "ntdll")
#pragma comment(lib, "ole32")
#pragma comment(lib, "OneCoreUAP")
#pragma comment(lib, "powrprof")
#pragma comment(lib, "rpcrt4")
#pragma comment(lib, "setupapi")
#pragma comment(lib, "shcore")
#pragma comment(lib, "shell32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "winmm")
#pragma comment(lib, "winusb")
#pragma comment(lib, "user32")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dbghelp")
#pragma comment(lib, "imagehlp")
#pragma comment(lib, "opengl32")
#pragma comment(lib, "winmm")

end_c

