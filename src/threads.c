#include "runtime.h"
#include "win32.h"

static void* threads_ntdll(void) {
    static HMODULE ntdll;
    if (ntdll == null) {
        ntdll = (void*)GetModuleHandleA("ntdll.dll");
    }
    if (ntdll == null) {
        ntdll = loader.open("ntdll.dll", 0);
    }
    not_null(ntdll);
    return ntdll;
}


static double threads_ns2ms(int64_t ns) {
    return ns / (double)nsec_in_msec;
}

static int threads_scheduler_set_timer_resolution(uint64_t nanoseconds) {
    typedef int (*gettimerresolution_t)(ULONG* minimum_resolution,
        ULONG* maximum_resolution, ULONG* actual_resolution);
    typedef int (*settimerresolution_t)(ULONG requested_resolution,
        BOOLEAN Set, ULONG* actual_resolution);    // ntdll.dll
    typedef int (*timeBeginPeriod_t)(UINT period); // winmm.dll
    void* ntdll = threads_ntdll();
    void* winmm = loader.open("winmm.dll", 0);
    not_null(winmm);
    gettimerresolution_t NtQueryTimerResolution =  (gettimerresolution_t)
        loader.sym(ntdll, "NtQueryTimerResolution");
    settimerresolution_t NtSetTimerResolution = (settimerresolution_t)
        loader.sym(ntdll, "NtSetTimerResolution");
    timeBeginPeriod_t timeBeginPeriod = (timeBeginPeriod_t)
        loader.sym(winmm, "timeBeginPeriod");
    unsigned long min100ns = 16 * 10 * 1000;
    unsigned long max100ns =  1 * 10 * 1000;
    unsigned long cur100ns =  0;
    int r = 0;
    if (NtQueryTimerResolution != null &&
        NtQueryTimerResolution(&min100ns, &max100ns, &cur100ns) == 0) {
        uint64_t min_ns = min100ns * 100uLL;
        uint64_t max_ns = max100ns * 100uLL; // lowest possible delay between timer events
        uint64_t cur_ns = cur100ns * 100uLL;
        if (debug.verbosity >= debug.trace) {
            traceln("timer resolution min: %.3f max: %.3f cur: %.3f ms (milliseconds)",
                threads_ns2ms(min_ns),
                threads_ns2ms(max_ns),
                threads_ns2ms(cur_ns));
        }
        // note that maximum resolution is actually < minimum
        nanoseconds = maximum(max_ns, nanoseconds);
        if (NtSetTimerResolution == null) {
            const int milliseconds = (int)(threads_ns2ms(nanoseconds) + 0.5);
            r = timeBeginPeriod(milliseconds);
        } else {
            unsigned long ns = (unsigned long)((nanoseconds + 99) / 100);
            r = NtSetTimerResolution(ns, true, &cur100ns);
        }
        fatal_if(NtQueryTimerResolution(&min100ns, &max100ns, &cur100ns) != 0);
        if (debug.verbosity >= debug.trace) {
            min_ns = min100ns * 100uLL;
            max_ns = max100ns * 100uLL; // the smallest interval
            cur_ns = cur100ns * 100uLL;
            traceln("timer resolution min: %.3f max: %.3f cur: %.3f ms (milliseconds)",
                threads_ns2ms(min_ns),
                threads_ns2ms(max_ns),
                threads_ns2ms(cur_ns));
        }
    } else {
        const int milliseconds = (int)(threads_ns2ms(nanoseconds) + 0.5);
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

static const char* threads_rel2str(int rel) {
    switch (rel) {
        case RelationProcessorCore   : return "ProcessorCore   ";
        case RelationNumaNode        : return "NumaNode        ";
        case RelationCache           : return "Cache           ";
        case RelationProcessorPackage: return "ProcessorPackage";
        case RelationGroup           : return "Group           ";
        case RelationProcessorDie    : return "ProcessorDie    ";
        case RelationNumaNodeEx      : return "NumaNodeEx      ";
        case RelationProcessorModule : return "ProcessorModule ";
        default: assert(false, "fix me"); return "???";
    }
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
            if (debug.verbosity >= debug.trace) {
                traceln("[%2d] affinity mask 0x%016llX relationship=%d %s", i,
                    lpi[i].ProcessorMask, lpi[i].Relationship,
                    threads_rel2str(lpi[i].Relationship));
            }
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
    fatal_if_not_zero( // desired: 1us (microsecond)
        threads_scheduler_set_timer_resolution(nsec_in_usec));
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
            loader.sym(ntdll, "NtDelayExecution");
        not_null(NtDelayExecution);
    }
    // If "alertable" is set, sleep_for() can break earlier
    // as a result of NtAlertThread call.
    NtDelayExecution(false, &delay);
}

static int32_t threads_id(void) { return GetThreadId(GetCurrentThread()); }

// test: https://en.wikipedia.org/wiki/Dining_philosophers_problem

typedef struct threads_philosophers_s threads_philosophers_t;

typedef struct {
    threads_philosophers_t* ps;
    mutex_t  fork;
    mutex_t* left_fork;
    mutex_t* right_fork;
    thread_t thread;
    int32_t  id;
} threads_philosopher_t;

typedef struct threads_philosophers_s {
    threads_philosopher_t philosopher[3];
    event_t fed_up[3];
    int32_t verbosity;
    uint32_t seed;
    volatile bool enough;
} threads_philosophers_t;

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                           \
    if (p->ps->verbosity >= debug.trace) { traceln(__VA_ARGS__); }  \
} while (0)

static void threads_philosopher_think(threads_philosopher_t* p) {
    verbose("philosopher %d is thinking.", p->id);
    // Random think time between .1 and .3 seconds
    double seconds = (num.random32(&p->ps->seed) % 30 + 1) / 100.0;
    threads.sleep_for(seconds);
}

static void threads_philosopher_eat(threads_philosopher_t* p) {
    verbose("philosopher %d is eating.", p->id);
    // Random eat time between .1 and .2 seconds
    double seconds = (num.random32(&p->ps->seed) % 20 + 1) / 100.0;
    threads.sleep_for(seconds);
}

// To avoid deadlocks in the Three Philosophers problem, we can implement
// the Tanenbaum's solution, which ensures that one of the philosophers
// (e.g., the last one) tries to pick up the right fork first, while the
// others pick up the left fork first. This breaks the circular wait
// condition and prevents deadlock.

// If the philosopher is the last one (p->id == n - 1) they will try to pick
// up the right fork first and then the left fork. All other philosophers will
// pick up the left fork first and then the right fork, as before. This change
// ensures that at least one philosopher will be able to eat, breaking the
// circular wait condition and preventing deadlock.

static void threads_philosopher_routine(void* arg) {
    threads_philosopher_t* p = (threads_philosopher_t*)arg;
    enum { n = countof(p->ps->philosopher) };
    threads.name("philosopher");
    threads.realtime();
    while (!p->ps->enough) {
        threads_philosopher_think(p);
        if (p->id == n - 1) { // Last philosopher picks up the right fork first
            mutexes.lock(p->right_fork);
            verbose("philosopher %d picked up right fork.", p->id);
            mutexes.lock(p->left_fork);
            verbose("philosopher %d picked up left fork.", p->id);
        } else { // Other philosophers pick up the left fork first
            mutexes.lock(p->left_fork);
            verbose("philosopher %d picked up left fork.", p->id);
            mutexes.lock(p->right_fork);
            verbose("philosopher %d picked up right fork.", p->id);
        }
        threads_philosopher_eat(p);
        mutexes.unlock(p->right_fork);
        verbose("philosopher %d put down right fork.", p->id);
        mutexes.unlock(p->left_fork);
        verbose("philosopher %d put down left fork.", p->id);
        events.set(p->ps->fed_up[p->id]);
    }
}

static void threads_test(int32_t verbosity) {
    threads_philosophers_t ps = {
        .verbosity = verbosity,
        .seed = 1
    };
    enum { n = countof(ps.philosopher) };
    // Initialize mutexes for forks
    for (int i = 0; i < n; i++) {
        threads_philosopher_t* p = &ps.philosopher[i];
        p->id = i;
        p->ps = &ps;
        mutexes.init(&p->fork);
        p->left_fork = &p->fork;
        ps.fed_up[i] = events.create();
    }
    // Create and start philosopher threads
    for (int i = 0; i < n; i++) {
        threads_philosopher_t* p = &ps.philosopher[i];
        threads_philosopher_t* r = &ps.philosopher[(i + 1) % n];
        p->right_fork = r->left_fork;
        p->thread = threads.start(threads_philosopher_routine, p);
    }
    // wait for all philosophers being fed up:
    for (int i = 0; i < n; i++) { events.wait(ps.fed_up[i]); }
    ps.enough = true;
    // join all philosopher threads
    for (int i = 0; i < n; i++) {
        threads_philosopher_t* p = &ps.philosopher[i];
        threads.join(p->thread);
    }
    // Dispose of mutexes and events
    for (int i = 0; i < n; ++i) {
        threads_philosopher_t* p = &ps.philosopher[i];
        mutexes.dispose(&p->fork);
        events.dispose(ps.fed_up[i]);
    }
    if (verbosity > 0) { traceln("done"); }
}

#pragma pop_macro("verbose")

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
