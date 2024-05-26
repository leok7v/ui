#include "ut/ut.h"
#include "ut/ut_win32.h"

// events:

static event_t ut_event_create(void) {
    HANDLE e = CreateEvent(null, false, false, null);
    not_null(e);
    return (event_t)e;
}

static event_t ut_event_create_manual(void) {
    HANDLE e = CreateEvent(null, true, false, null);
    not_null(e);
    return (event_t)e;
}

static void ut_event_set(event_t e) {
    fatal_if_false(SetEvent((HANDLE)e));
}

static void ut_event_reset(event_t e) {
    fatal_if_false(ResetEvent((HANDLE)e));
}

static int32_t ut_event_wait_or_timeout(event_t e, fp64_t seconds) {
    uint32_t ms = seconds < 0 ? INFINITE : (uint32_t)(seconds * 1000.0 + 0.5);
    DWORD ix = WaitForSingleObject(e, ms);
    errno_t r = wait2e(ix);
    return r != 0 ? -1 : 0;
}

static void ut_event_wait(event_t e) { ut_event_wait_or_timeout(e, -1); }

static int32_t ut_event_wait_any_or_timeout(int32_t n, event_t events[],
        fp64_t s) {
    const uint32_t ms = s < 0 ? INFINITE : (uint32_t)(s * 1000.0 + 0.5);
    const HANDLE* es = (const HANDLE*)events;
    DWORD ix = WaitForMultipleObjects((DWORD)n, es, false, ms);
    errno_t r = wait2e(ix);
    // all WAIT_ABANDONED_0 and WAIT_IO_COMPLETION 0xC0 as -1
    return r != 0 ? -1 : (int32_t)ix;
}

static int32_t ut_event_wait_any(int32_t n, event_t e[]) {
    return ut_event_wait_any_or_timeout(n, e, -1);
}

static void ut_event_dispose(event_t handle) {
    fatal_if_false(CloseHandle(handle));
}

// test:

// check if the elapsed time is within the expected range
static void ut_event_test_check_time(fp64_t start, fp64_t expected) {
    fp64_t elapsed = ut_clock.seconds() - start;
    // Old Windows scheduler is prone to 2x16.6ms ~ 33ms delays
    swear(elapsed >= expected - 0.04 && elapsed <= expected + 0.04,
          "expected: %f elapsed %f seconds", expected, elapsed);
}

static void ut_event_test(void) {
    #ifdef UT_TESTS
    event_t event = ut_event.create();
    fp64_t start = ut_clock.seconds();
    ut_event.set(event);
    ut_event.wait(event);
    ut_event_test_check_time(start, 0); // Event should be immediate
    ut_event.reset(event);
    start = ut_clock.seconds();
    const fp64_t timeout_seconds = 0.01;
    int32_t result = ut_event.wait_or_timeout(event, timeout_seconds);
    ut_event_test_check_time(start, timeout_seconds);
    swear(result == -1); // Timeout expected
    enum { count = 5 };
    event_t event_array[count];
    for (int32_t i = 0; i < countof(event_array); i++) {
        event_array[i] = ut_event.create_manual();
    }
    start = ut_clock.seconds();
    ut_event.set(event_array[2]); // Set the third event
    int32_t index = ut_event.wait_any(countof(event_array), event_array);
    ut_event_test_check_time(start, 0);
    swear(index == 2); // Third event should be triggered
    ut_event.reset(event_array[2]); // Reset the third event
    start = ut_clock.seconds();
    result = ut_event.wait_any_or_timeout(countof(event_array),
        event_array, timeout_seconds);
    ut_event_test_check_time(start, timeout_seconds);
    swear(result == -1); // Timeout expected
    // Clean up
    ut_event.dispose(event);
    for (int32_t i = 0; i < countof(event_array); i++) {
        ut_event.dispose(event_array[i]);
    }
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #endif
}

ut_event_if ut_event = {
    .create              = ut_event_create,
    .create_manual       = ut_event_create_manual,
    .set                 = ut_event_set,
    .reset               = ut_event_reset,
    .wait                = ut_event_wait,
    .wait_or_timeout     = ut_event_wait_or_timeout,
    .wait_any            = ut_event_wait_any,
    .wait_any_or_timeout = ut_event_wait_any_or_timeout,
    .dispose             = ut_event_dispose,
    .test                = ut_event_test
};

// mutexes:

static_assertion(sizeof(CRITICAL_SECTION) == sizeof(mutex_t));

static void ut_mutex_init(mutex_t* m) {
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)m;
    fatal_if_false(
        InitializeCriticalSectionAndSpinCount(cs, 4096)
    );
}

static void ut_mutex_lock(mutex_t* m) { EnterCriticalSection((CRITICAL_SECTION*)m); }

static void ut_mutex_unlock(mutex_t* m) { LeaveCriticalSection((CRITICAL_SECTION*)m); }

static void ut_mutex_dispose(mutex_t* m) { DeleteCriticalSection((CRITICAL_SECTION*)m); }

// test:

// check if the elapsed time is within the expected range
static void ut_mutex_test_check_time(fp64_t start, fp64_t expected) {
    fp64_t elapsed = ut_clock.seconds() - start;
    // Old Windows scheduler is prone to 2x16.6ms ~ 33ms delays
    swear(elapsed >= expected - 0.04 && elapsed <= expected + 0.04,
          "expected: %f elapsed %f seconds", expected, elapsed);
}

static void ut_mutex_test_lock_unlock(void* arg) {
    mutex_t* mutex = (mutex_t*)arg;
    ut_mutex.lock(mutex);
    ut_thread.sleep_for(0.01); // Hold the mutex for 10ms
    ut_mutex.unlock(mutex);
}

static void ut_mutex_test(void) {
    mutex_t mutex;
    ut_mutex.init(&mutex);
    fp64_t start = ut_clock.seconds();
    ut_mutex.lock(&mutex);
    ut_mutex.unlock(&mutex);
    // Lock and unlock should be immediate
    ut_mutex_test_check_time(start, 0);
    enum { count = 5 };
    thread_t ts[count];
    for (int32_t i = 0; i < countof(ts); i++) {
        ts[i] = ut_thread.start(ut_mutex_test_lock_unlock, &mutex);
    }
    // Wait for all threads to finish
    for (int32_t i = 0; i < countof(ts); i++) {
        ut_thread.join(ts[i], -1);
    }
    ut_mutex.dispose(&mutex);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

ut_mutex_if ut_mutex = {
    .init    = ut_mutex_init,
    .lock    = ut_mutex_lock,
    .unlock  = ut_mutex_unlock,
    .dispose = ut_mutex_dispose,
    .test    = ut_mutex_test
};

// threads:

static void* ut_thread_ntdll(void) {
    static HMODULE ntdll;
    if (ntdll == null) {
        ntdll = (void*)GetModuleHandleA("ntdll.dll");
    }
    if (ntdll == null) {
        ntdll = ut_loader.open("ntdll.dll", 0);
    }
    not_null(ntdll);
    return ntdll;
}

static fp64_t ut_thread_ns2ms(int64_t ns) {
    return (fp64_t)ns / (fp64_t)ut_clock.nsec_in_msec;
}

static void ut_thread_set_timer_resolution(uint64_t nanoseconds) {
    typedef int32_t (*query_timer_resolution_t)(ULONG* minimum_resolution,
        ULONG* maximum_resolution, ULONG* actual_resolution);
    typedef int32_t (*set_timer_resolution_t)(ULONG requested_resolution,
        BOOLEAN set, ULONG* actual_resolution); // ntdll.dll
    void* nt_dll = ut_thread_ntdll();
    query_timer_resolution_t query_timer_resolution =  (query_timer_resolution_t)
        ut_loader.sym(nt_dll, "NtQueryTimerResolution");
    set_timer_resolution_t set_timer_resolution = (set_timer_resolution_t)
        ut_loader.sym(nt_dll, "NtSetTimerResolution");
    unsigned long min100ns = 16 * 10 * 1000;
    unsigned long max100ns =  1 * 10 * 1000;
    unsigned long cur100ns =  0;
    fatal_if(query_timer_resolution(&min100ns, &max100ns, &cur100ns) != 0);
    uint64_t max_ns = max100ns * 100uLL;
//  uint64_t min_ns = min100ns * 100uLL;
//  uint64_t cur_ns = cur100ns * 100uLL;
    // max resolution is lowest possible delay between timer events
//  if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
//      traceln("timer resolution min: %.3f max: %.3f cur: %.3f"
//          " ms (milliseconds)",
//          ut_thread_ns2ms(min_ns),
//          ut_thread_ns2ms(max_ns),
//          ut_thread_ns2ms(cur_ns));
//  }
    // note that maximum resolution is actually < minimum
    nanoseconds = ut_max(max_ns, nanoseconds);
    unsigned long ns = (unsigned long)((nanoseconds + 99) / 100);
    fatal_if(set_timer_resolution(ns, true, &cur100ns) != 0);
    fatal_if(query_timer_resolution(&min100ns, &max100ns, &cur100ns) != 0);
//  if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
//      min_ns = min100ns * 100uLL;
//      max_ns = max100ns * 100uLL; // the smallest interval
//      cur_ns = cur100ns * 100uLL;
//      traceln("timer resolution min: %.3f max: %.3f cur: %.3f ms (milliseconds)",
//          ut_thread_ns2ms(min_ns),
//          ut_thread_ns2ms(max_ns),
//          ut_thread_ns2ms(cur_ns));
//  }
}

static void ut_thread_power_throttling_disable_for_process(void) {
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

static void ut_thread_power_throttling_disable_for_thread(HANDLE thread) {
    THREAD_POWER_THROTTLING_STATE pt = { 0 };
    pt.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
    pt.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    pt.StateMask = 0;
    fatal_if_false(SetThreadInformation(thread, ThreadPowerThrottling,
        &pt, sizeof(pt)));
}

static void ut_thread_disable_power_throttling(void) {
    ut_thread_power_throttling_disable_for_process();
    ut_thread_power_throttling_disable_for_thread(GetCurrentThread());
}

static const char* ut_thread_rel2str(int32_t rel) {
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

static uint64_t ut_thread_next_physical_processor_affinity_mask(void) {
    static volatile int32_t initialized;
    static int32_t init;
    static int32_t next = 1; // next physical core to use
    static int32_t cores = 0; // number of physical processors (cores)
    static uint64_t any;
    static uint64_t affinity[64]; // mask for each physical processor
    bool set_to_true = ut_atomics.compare_exchange_int32(&init, false, true);
    if (set_to_true) {
        // Concept D: 6 cores, 12 logical processors: 27 lpi entries
        static SYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi[64];
        DWORD bytes = 0;
        GetLogicalProcessorInformation(null, &bytes);
        assert(bytes % sizeof(lpi[0]) == 0);
        // number of lpi entries == 27 on 6 core / 12 logical processors system
        int32_t n = bytes / sizeof(lpi[0]);
        assert(bytes <= sizeof(lpi), "increase lpi[%d]", n);
        fatal_if_false(GetLogicalProcessorInformation(&lpi[0], &bytes));
        for (int32_t i = 0; i < n; i++) {
//          if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
//              traceln("[%2d] affinity mask 0x%016llX relationship=%d %s", i,
//                  lpi[i].ProcessorMask, lpi[i].Relationship,
//                  ut_thread_rel2str(lpi[i].Relationship));
//          }
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
        while (initialized == 0) { ut_thread.sleep_for(1 / 1024.0); }
        assert(any != 0); // should not ever happen
        if (any == 0) { any = (uint64_t)(-1LL); }
    }
    uint64_t mask = next < cores ? affinity[next] : any;
    assert(mask != 0);
    // assume last physical core is least popular
    if (next < cores) { next++; } // not circular
    return mask;
}

static void ut_thread_realtime(void) {
    fatal_if_false(SetPriorityClass(GetCurrentProcess(),
        REALTIME_PRIORITY_CLASS));
    fatal_if_false(SetThreadPriority(GetCurrentThread(),
        THREAD_PRIORITY_TIME_CRITICAL));
    fatal_if_false(SetThreadPriorityBoost(GetCurrentThread(),
        /* bDisablePriorityBoost = */ false));
    // desired: 0.5ms = 500us (microsecond) = 50,000ns
    ut_thread_set_timer_resolution((uint64_t)ut_clock.nsec_in_usec * 500ULL);
    fatal_if_false(SetThreadAffinityMask(GetCurrentThread(),
        ut_thread_next_physical_processor_affinity_mask()));
    ut_thread_disable_power_throttling();
}

static void ut_thread_yield(void) { SwitchToThread(); }

static thread_t ut_thread_start(void (*func)(void*), void* p) {
    thread_t t = (thread_t)CreateThread(null, 0,
        (LPTHREAD_START_ROUTINE)(void*)func, p, 0, null);
    not_null(t);
    return t;
}

static bool is_handle_valid(void* h) {
    DWORD flags = 0;
    return GetHandleInformation(h, &flags);
}

static errno_t ut_thread_join(thread_t t, fp64_t timeout) {
    not_null(t);
    fatal_if_false(is_handle_valid(t));
    const uint32_t ms = timeout < 0 ? INFINITE : (uint32_t)(timeout * 1000.0 + 0.5);
    DWORD ix = WaitForSingleObject(t, (DWORD)ms);
    errno_t r = wait2e(ix);
    assert(r != ERROR_REQUEST_ABORTED, "AFAIK thread can`t be ABANDONED");
    if (r == 0) {
        fatal_if_false(CloseHandle(t));
    } else {
        traceln("failed to join thread %p %s", t, ut_str.error(r));
    }
    return r;
}

static void ut_thread_detach(thread_t t) {
    not_null(t);
    fatal_if_false(is_handle_valid(t));
    fatal_if_false(CloseHandle(t));
}

static void ut_thread_name(const char* name) {
    uint16_t stack[1024];
    fatal_if(ut_str.length(name) >= countof(stack), "name too long: %s", name);
    uint16_t* wide = ut_str.utf8_utf16(stack, name);
    HRESULT r = SetThreadDescription(GetCurrentThread(), wide);
    // notoriously returns 0x10000000 for no good reason whatsoever
    if (!SUCCEEDED(r)) { fatal_if_not_zero(r); }
}

static void ut_thread_sleep_for(fp64_t seconds) {
    assert(seconds >= 0);
    if (seconds < 0) { seconds = 0; }
    int64_t ns100 = (int64_t)(seconds * 1.0e+7); // in 0.1 us aka 100ns
    typedef int32_t (__stdcall *nt_delay_execution_t)(BOOLEAN alertable,
        PLARGE_INTEGER DelayInterval);
    static nt_delay_execution_t NtDelayExecution;
    // delay in 100-ns units. negative value means delay relative to current.
    LARGE_INTEGER delay = {0}; // delay in 100-ns units.
    delay.QuadPart = -ns100; // negative value means delay relative to current.
    if (NtDelayExecution == null) {
        void* ntdll = ut_thread_ntdll();
        NtDelayExecution = (nt_delay_execution_t)
            ut_loader.sym(ntdll, "NtDelayExecution");
        not_null(NtDelayExecution);
    }
    // If "alertable" is set, sleep_for() can break earlier
    // as a result of NtAlertThread call.
    NtDelayExecution(false, &delay);
}

static int32_t ut_thread_id(void) {
    return (int32_t)GetThreadId(GetCurrentThread());
}

#ifdef UT_TESTS

// test: https://en.wikipedia.org/wiki/Dining_philosophers_problem

typedef struct ut_thread_philosophers_s ut_thread_philosophers_t;

typedef struct {
    ut_thread_philosophers_t* ps;
    mutex_t  fork;
    mutex_t* left_fork;
    mutex_t* right_fork;
    thread_t thread;
    int32_t  id;
} ut_thread_philosopher_t;

typedef struct ut_thread_philosophers_s {
    ut_thread_philosopher_t philosopher[3];
    event_t fed_up[3];
    uint32_t seed;
    volatile bool enough;
} ut_thread_philosophers_t;

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                 \
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) { \
        traceln(__VA_ARGS__);                             \
    }                                                     \
} while (0)

static void ut_thread_philosopher_think(ut_thread_philosopher_t* p) {
    verbose("philosopher %d is thinking.", p->id);
    // Random think time between .1 and .3 seconds
    fp64_t seconds = (ut_num.random32(&p->ps->seed) % 30 + 1) / 100.0;
    ut_thread.sleep_for(seconds);
}

static void ut_thread_philosopher_eat(ut_thread_philosopher_t* p) {
    verbose("philosopher %d is eating.", p->id);
    // Random eat time between .1 and .2 seconds
    fp64_t seconds = (ut_num.random32(&p->ps->seed) % 20 + 1) / 100.0;
    ut_thread.sleep_for(seconds);
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

static void ut_thread_philosopher_routine(void* arg) {
    ut_thread_philosopher_t* p = (ut_thread_philosopher_t*)arg;
    enum { n = countof(p->ps->philosopher) };
    ut_thread.name("philosopher");
    ut_thread.realtime();
    while (!p->ps->enough) {
        ut_thread_philosopher_think(p);
        if (p->id == n - 1) { // Last philosopher picks up the right fork first
            ut_mutex.lock(p->right_fork);
            verbose("philosopher %d picked up right fork.", p->id);
            ut_mutex.lock(p->left_fork);
            verbose("philosopher %d picked up left fork.", p->id);
        } else { // Other philosophers pick up the left fork first
            ut_mutex.lock(p->left_fork);
            verbose("philosopher %d picked up left fork.", p->id);
            ut_mutex.lock(p->right_fork);
            verbose("philosopher %d picked up right fork.", p->id);
        }
        ut_thread_philosopher_eat(p);
        ut_mutex.unlock(p->right_fork);
        verbose("philosopher %d put down right fork.", p->id);
        ut_mutex.unlock(p->left_fork);
        verbose("philosopher %d put down left fork.", p->id);
        ut_event.set(p->ps->fed_up[p->id]);
    }
}

static void ut_thread_detached_sleep(void* unused(p)) {
    ut_thread.sleep_for(1000.0); // seconds
}

static void ut_thread_detached_loop(void* unused(p)) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i < UINT64_MAX; i++) { sum += i; }
    // make sure that compiler won't get rid of the loop:
    swear(sum == 0x8000000000000001ULL, "sum: %llu 0x%16llX", sum, sum);
}

static void ut_thread_test(void) {
    ut_thread_philosophers_t ps = { .seed = 1 };
    enum { n = countof(ps.philosopher) };
    // Initialize mutexes for forks
    for (int32_t i = 0; i < n; i++) {
        ut_thread_philosopher_t* p = &ps.philosopher[i];
        p->id = i;
        p->ps = &ps;
        ut_mutex.init(&p->fork);
        p->left_fork = &p->fork;
        ps.fed_up[i] = ut_event.create();
    }
    // Create and start philosopher threads
    for (int32_t i = 0; i < n; i++) {
        ut_thread_philosopher_t* p = &ps.philosopher[i];
        ut_thread_philosopher_t* r = &ps.philosopher[(i + 1) % n];
        p->right_fork = r->left_fork;
        p->thread = ut_thread.start(ut_thread_philosopher_routine, p);
    }
    // wait for all philosophers being fed up:
    for (int32_t i = 0; i < n; i++) { ut_event.wait(ps.fed_up[i]); }
    ps.enough = true;
    // join all philosopher threads
    for (int32_t i = 0; i < n; i++) {
        ut_thread_philosopher_t* p = &ps.philosopher[i];
        ut_thread.join(p->thread, -1);
    }
    // Dispose of mutexes and events
    for (int32_t i = 0; i < n; ++i) {
        ut_thread_philosopher_t* p = &ps.philosopher[i];
        ut_mutex.dispose(&p->fork);
        ut_event.dispose(ps.fed_up[i]);
    }
    // detached threads are hacky and not that swell of an idea
    // but sometimes can be useful for 1. quick hacks 2. threads
    // that execute blocking calls that e.g. write logs to the
    // internet service that hangs.
    // test detached threads
    thread_t detached_sleep = ut_thread.start(ut_thread_detached_sleep, null);
    ut_thread.detach(detached_sleep);
    thread_t detached_loop = ut_thread.start(ut_thread_detached_loop, null);
    ut_thread.detach(detached_loop);
    // leave detached threads sleeping and running till ExitProcess(0)
    // that should NOT hang.
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#pragma pop_macro("verbose")

#else
static void ut_thread_test(void) { }
#endif

ut_thread_if ut_thread = {
    .start     = ut_thread_start,
    .join      = ut_thread_join,
    .detach    = ut_thread_detach,
    .name      = ut_thread_name,
    .realtime  = ut_thread_realtime,
    .yield     = ut_thread_yield,
    .sleep_for = ut_thread_sleep_for,
    .id        = ut_thread_id,
    .test      = ut_thread_test
};
