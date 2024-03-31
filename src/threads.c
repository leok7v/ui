#include "rt.h"
#include "win32.h"

static int64_t threads_ns2ms(int64_t ns) { return (ns + nsec_in_msec - 1) / nsec_in_msec; }

static void* threads_ntdll(void) {
    static HMODULE ntdll;
    if (ntdll == null) {
        ntdll = (void*)GetModuleHandleA("ntdll.dll");
    }
    if (ntdll == null) {
        ntdll = crt.dlopen("ntdll.dll", 0);
    }
    not_null(ntdll);
    return ntdll;
}

typedef int (*gettimerresolution_t)(ULONG* minimum_resolution,
    ULONG* maximum_resolution, ULONG* actual_resolution);
typedef int (*settimerresolution_t)(ULONG requested_resolution,
    BOOLEAN Set, ULONG* actual_resolution);  // ntdll.dll
typedef int (*timeBeginPeriod_t)(UINT period); // winmm.dll

static int threads_scheduler_set_timer_resolution(int64_t ns) { // nanoseconds
    const int ns100 = (int)(ns / 100);
    void* ntdll = threads_ntdll();
    void* winmm = crt.dlopen("winmm.dll", 0);
    not_null(winmm);
    gettimerresolution_t NtQueryTimerResolution =  (gettimerresolution_t)
        crt.dlsym(ntdll, "NtQueryTimerResolution");
    settimerresolution_t NtSetTimerResolution = (settimerresolution_t)
        crt.dlsym(ntdll, "NtSetTimerResolution");
    timeBeginPeriod_t timeBeginPeriod = (timeBeginPeriod_t)
        crt.dlsym(winmm, "timeBeginPeriod");
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
        if (NtSetTimerResolution == null) {
            const int milliseconds = (int)(threads_ns2ms(ns) + 0.5);
            r = (int)maximum_ns <= ns && ns <= (int)minimum_ns ?
                timeBeginPeriod(milliseconds) : ERROR_INVALID_PARAMETER;
        } else {
            r = (int)maximum_ns <= ns && ns <= (int)minimum_ns ?
                NtSetTimerResolution(ns100, true, &actual_100ns) :
                ERROR_INVALID_PARAMETER;
        }
        NtQueryTimerResolution(&min_100ns, &max_100ns, &actual_100ns);
    } else {
        const int milliseconds = (int)(threads_ns2ms(ns) + 0.5);
        r = 1 <= milliseconds && milliseconds <= 16 ?
            timeBeginPeriod(milliseconds) : ERROR_INVALID_PARAMETER;
    }
    return r;
}

static void threads_power_throttling_disable_for_process(void) {
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

static void threads_power_throttling_disable_for_thread(HANDLE thread) {
    THREAD_POWER_THROTTLING_STATE pt = { 0 };
    pt.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
    pt.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    pt.StateMask = 0;
    fatal_if_false(SetThreadInformation(thread, ThreadPowerThrottling,
        &pt, sizeof(pt)));
}

static void threads_disable_power_throttling(void) {
    threads_power_throttling_disable_for_process();
    threads_power_throttling_disable_for_thread(GetCurrentThread());
}

static uint64_t threads_next_physical_processor_affinity_mask(void) {
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
        while (initialized == 0) { threads.sleep_for(1 / 1024.0); }
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
        threads_scheduler_set_timer_resolution(nsec_in_msec));
    fatal_if_false(SetThreadAffinityMask(GetCurrentThread(),
        threads_next_physical_processor_affinity_mask()));
    threads_disable_power_throttling();
}

static void threads_yield(void) { SwitchToThread(); }

static thread_t threads_start(void (*func)(void*), void* p) {
    thread_t t = (thread_t)CreateThread(null, 0,
        (LPTHREAD_START_ROUTINE)(void*)func, p, 0, null);
    not_null(t);
    return t;
}

static bool is_handle_valid(void* h) {
    DWORD flags = 0;
    return GetHandleInformation(h, &flags);
}

static bool threads_try_join(thread_t t, double timeout) {
    not_null(t);
    fatal_if_false(is_handle_valid(t));
    int32_t timeout_ms = timeout <= 0 ? 0 : (int)(timeout * 1000.0 + 0.5);
    int r = WaitForSingleObject(t, timeout_ms);
    if (r != 0) {
        traceln("failed to join thread %p %s", t, str.error(r));
    } else {
        r = CloseHandle(t) ? 0 : GetLastError();
        if (r != 0) { traceln("CloseHandle(%p) failed %s", t, str.error(r)); }
    }
    return r == 0;
}

static void threads_join(thread_t t) {
    // longer timeout for super slow instrumented code
    #ifdef DEBUG
    const double timeout = 3.0; // 30 seconds
    #else
    const double timeout = 1.0; // 1 second
    #endif
    if (!threads.try_join(t, timeout)) {
        traceln("failed to join thread %p", t);
    }
}

static void threads_name(const char* name) {
    HRESULT r = SetThreadDescription(GetCurrentThread(), utf8to16(name));
    // notoriously returns 0x10000000 for no good reason whatsoever
    if (!SUCCEEDED(r)) { fatal_if_not_zero(r); }
}

static void threads_sleep_for(double seconds) {
    assert(seconds >= 0);
    if (seconds < 0) { seconds = 0; }
    int64_t ns100 = (int64_t)(seconds * 1.0e+7); // in 0.1 us aka 100ns
    typedef int (__stdcall *nt_delay_execution_t)(BOOLEAN alertable,
        PLARGE_INTEGER DelayInterval);
    static nt_delay_execution_t NtDelayExecution;
    // delay in 100-ns units. negative value means delay relative to current.
    LARGE_INTEGER delay = {0}; // delay in 100-ns units.
    delay.QuadPart = -ns100; // negative value means delay relative to current.
    if (NtDelayExecution == null) {
        void* ntdll = threads_ntdll();
        NtDelayExecution = (nt_delay_execution_t)
            crt.dlsym(ntdll, "NtDelayExecution");
        not_null(NtDelayExecution);
    }
    // If "alertable" is set, sleep_for() can break earlier
    // as a result of NtAlertThread call.
    NtDelayExecution(false, &delay);
}

static int32_t threads_id(void) { return GetThreadId(GetCurrentThread()); }

static void threads_test(int32_t verbosity) {
    // TODO: implement me
    if (verbosity > 0) { traceln("done"); }
}

threads_if threads = {
    .start     = threads_start,
    .try_join  = threads_try_join,
    .join      = threads_join,
    .name      = threads_name,
    .realtime  = threads_realtime,
    .yield     = threads_yield,
    .sleep_for = threads_sleep_for,
    .id        = threads_id,
    .test      = threads_test
};