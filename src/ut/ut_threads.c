#include "ut/ut.h"
#include "ut/ut_win32.h"

// events:

static event_t events_create(void) {
    HANDLE e = CreateEvent(null, false, false, null);
    not_null(e);
    return (event_t)e;
}

static event_t events_create_manual(void) {
    HANDLE e = CreateEvent(null, true, false, null);
    not_null(e);
    return (event_t)e;
}

static void events_set(event_t e) {
    fatal_if_false(SetEvent((HANDLE)e));
}

static void events_reset(event_t e) {
    fatal_if_false(ResetEvent((HANDLE)e));
}

static int32_t events_wait_or_timeout(event_t e, fp64_t seconds) {
    uint32_t ms = seconds < 0 ? INFINITE : (int32_t)(seconds * 1000.0 + 0.5);
    DWORD ix = WaitForSingleObject(e, ms);
    errno_t r = wait2e(ix);
    return r != 0 ? -1 : 0;
}

static void events_wait(event_t e) { events_wait_or_timeout(e, -1); }

static int32_t events_wait_any_or_timeout(int32_t n, event_t events_[], fp64_t s) {
    uint32_t ms = s < 0 ? INFINITE : (int32_t)(s * 1000.0 + 0.5);
    DWORD ix = WaitForMultipleObjects(n, events_, false, ms);
    errno_t r = wait2e(ix);
    // all WAIT_ABANDONED_0 and WAIT_IO_COMPLETION 0xC0 as -1
    return r != 0 ? -1 : ix;
}

static int32_t events_wait_any(int32_t n, event_t e[]) {
    return events_wait_any_or_timeout(n, e, -1);
}

static void events_dispose(event_t handle) {
    fatal_if_false(CloseHandle(handle));
}

// test:

// check if the elapsed time is within the expected range
static void events_test_check_time(fp64_t start, fp64_t expected) {
    fp64_t elapsed = ut_clock.seconds() - start;
    // Old Windows scheduler is prone to 2x16.6ms ~ 33ms delays
    swear(elapsed >= expected - 0.04 && elapsed <= expected + 0.04,
          "expected: %f elapsed %f seconds", expected, elapsed);
}

static void events_test(void) {
    #ifdef UT_TESTS
    event_t event = events.create();
    fp64_t start = ut_clock.seconds();
    events.set(event);
    events.wait(event);
    events_test_check_time(start, 0); // Event should be immediate
    events.reset(event);
    start = ut_clock.seconds();
    const fp64_t timeout_seconds = 0.01;
    int32_t result = events.wait_or_timeout(event, timeout_seconds);
    events_test_check_time(start, timeout_seconds);
    swear(result == -1); // Timeout expected
    enum { count = 5 };
    event_t event_array[count];
    for (int32_t i = 0; i < countof(event_array); i++) {
        event_array[i] = events.create_manual();
    }
    start = ut_clock.seconds();
    events.set(event_array[2]); // Set the third event
    int32_t index = events.wait_any(countof(event_array), event_array);
    events_test_check_time(start, 0);
    swear(index == 2); // Third event should be triggered
    events.reset(event_array[2]); // Reset the third event
    start = ut_clock.seconds();
    result = events.wait_any_or_timeout(countof(event_array),
        event_array, timeout_seconds);
    events_test_check_time(start, timeout_seconds);
    swear(result == -1); // Timeout expected
    // Clean up
    events.dispose(event);
    for (int32_t i = 0; i < countof(event_array); i++) {
        events.dispose(event_array[i]);
    }
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
    #endif
}

events_if events = {
    .create              = events_create,
    .create_manual       = events_create_manual,
    .set                 = events_set,
    .reset               = events_reset,
    .wait                = events_wait,
    .wait_or_timeout     = events_wait_or_timeout,
    .wait_any            = events_wait_any,
    .wait_any_or_timeout = events_wait_any_or_timeout,
    .dispose             = events_dispose,
    .test                = events_test
};

// mutexes:

static_assertion(sizeof(CRITICAL_SECTION) == sizeof(mutex_t));

static void mutexes_init(mutex_t* m) {
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)m;
    fatal_if_false(
        InitializeCriticalSectionAndSpinCount(cs, 4096)
    );
}

static void mutexes_lock(mutex_t* m) { EnterCriticalSection((CRITICAL_SECTION*)m); }

static void mutexes_unlock(mutex_t* m) { LeaveCriticalSection((CRITICAL_SECTION*)m); }

static void mutexes_dispose(mutex_t* m) { DeleteCriticalSection((CRITICAL_SECTION*)m); }

// test:

// check if the elapsed time is within the expected range
static void mutexes_test_check_time(fp64_t start, fp64_t expected) {
    fp64_t elapsed = ut_clock.seconds() - start;
    // Old Windows scheduler is prone to 2x16.6ms ~ 33ms delays
    swear(elapsed >= expected - 0.04 && elapsed <= expected + 0.04,
          "expected: %f elapsed %f seconds", expected, elapsed);
}

static void mutexes_test_lock_unlock(void* arg) {
    mutex_t* mutex = (mutex_t*)arg;
    mutexes.lock(mutex);
    threads.sleep_for(0.01); // Hold the mutex for 10ms
    mutexes.unlock(mutex);
}

static void mutexes_test(void) {
    mutex_t mutex;
    mutexes.init(&mutex);
    fp64_t start = ut_clock.seconds();
    mutexes.lock(&mutex);
    mutexes.unlock(&mutex);
    // Lock and unlock should be immediate
    mutexes_test_check_time(start, 0);
    enum { count = 5 };
    thread_t ts[count];
    for (int32_t i = 0; i < countof(ts); i++) {
        ts[i] = threads.start(mutexes_test_lock_unlock, &mutex);
    }
    // Wait for all threads to finish
    for (int32_t i = 0; i < countof(ts); i++) {
        threads.join(ts[i], -1);
    }
    mutexes.dispose(&mutex);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

mutex_if mutexes = {
    .init    = mutexes_init,
    .lock    = mutexes_lock,
    .unlock  = mutexes_unlock,
    .dispose = mutexes_dispose,
    .test    = mutexes_test
};

// threads:

static void* threads_ntdll(void) {
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

static fp64_t threads_ns2ms(int64_t ns) {
    return ns / (fp64_t)ut_clock.nsec_in_msec;
}

static void threads_set_timer_resolution(uint64_t nanoseconds) {
    typedef int32_t (*query_timer_resolution_t)(ULONG* minimum_resolution,
        ULONG* maximum_resolution, ULONG* actual_resolution);
    typedef int32_t (*set_timer_resolution_t)(ULONG requested_resolution,
        BOOLEAN set, ULONG* actual_resolution); // ntdll.dll
    void* nt_dll = threads_ntdll();
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
//          threads_ns2ms(min_ns),
//          threads_ns2ms(max_ns),
//          threads_ns2ms(cur_ns));
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
//          threads_ns2ms(min_ns),
//          threads_ns2ms(max_ns),
//          threads_ns2ms(cur_ns));
//  }
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

static const char* threads_rel2str(int32_t rel) {
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
//                  threads_rel2str(lpi[i].Relationship));
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
    // desired: 0.5ms = 500us (microsecond) = 50,000ns
    threads_set_timer_resolution(ut_clock.nsec_in_usec * 500);
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

static errno_t threads_join(thread_t t, fp64_t timeout) {
    not_null(t);
    fatal_if_false(is_handle_valid(t));
    int32_t timeout_ms = timeout < 0 ? INFINITE : (int)(timeout * 1000.0 + 0.5);
    DWORD ix = WaitForSingleObject(t, timeout_ms);
    errno_t r = wait2e(ix);
    assert(r != ERROR_REQUEST_ABORTED, "AFAIK thread can`t be ABANDONED");
    if (r == 0) {
        fatal_if_false(CloseHandle(t));
    } else {
        traceln("failed to join thread %p %s", t, ut_str.error(r));
    }
    return r;
}

static void threads_detach(thread_t t) {
    not_null(t);
    fatal_if_false(is_handle_valid(t));
    fatal_if_false(CloseHandle(t));
}

static void threads_name(const char* name) {
    uint16_t stack[1024];
    fatal_if(ut_str.length(name) >= countof(stack), "name too long: %s", name);
    uint16_t* wide = ut_str.utf8_utf16(stack, name);
    HRESULT r = SetThreadDescription(GetCurrentThread(), wide);
    // notoriously returns 0x10000000 for no good reason whatsoever
    if (!SUCCEEDED(r)) { fatal_if_not_zero(r); }
}

static void threads_sleep_for(fp64_t seconds) {
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
        void* ntdll = threads_ntdll();
        NtDelayExecution = (nt_delay_execution_t)
            ut_loader.sym(ntdll, "NtDelayExecution");
        not_null(NtDelayExecution);
    }
    // If "alertable" is set, sleep_for() can break earlier
    // as a result of NtAlertThread call.
    NtDelayExecution(false, &delay);
}

static int32_t threads_id(void) { return GetThreadId(GetCurrentThread()); }

#ifdef UT_TESTS

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
    uint32_t seed;
    volatile bool enough;
} threads_philosophers_t;

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                 \
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) { \
        traceln(__VA_ARGS__);                             \
    }                                                     \
} while (0)

static void threads_philosopher_think(threads_philosopher_t* p) {
    verbose("philosopher %d is thinking.", p->id);
    // Random think time between .1 and .3 seconds
    fp64_t seconds = (ut_num.random32(&p->ps->seed) % 30 + 1) / 100.0;
    threads.sleep_for(seconds);
}

static void threads_philosopher_eat(threads_philosopher_t* p) {
    verbose("philosopher %d is eating.", p->id);
    // Random eat time between .1 and .2 seconds
    fp64_t seconds = (ut_num.random32(&p->ps->seed) % 20 + 1) / 100.0;
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

static void threads_detached_sleep(void* unused(p)) {
    threads.sleep_for(1000.0); // seconds
}

static void threads_detached_loop(void* unused(p)) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i < UINT64_MAX; i++) { sum += i; }
    // making sure that compiler won't get rid of the loop:
    traceln("%lld", sum);
}

static void threads_test(void) {
    threads_philosophers_t ps = { .seed = 1 };
    enum { n = countof(ps.philosopher) };
    // Initialize mutexes for forks
    for (int32_t i = 0; i < n; i++) {
        threads_philosopher_t* p = &ps.philosopher[i];
        p->id = i;
        p->ps = &ps;
        mutexes.init(&p->fork);
        p->left_fork = &p->fork;
        ps.fed_up[i] = events.create();
    }
    // Create and start philosopher threads
    for (int32_t i = 0; i < n; i++) {
        threads_philosopher_t* p = &ps.philosopher[i];
        threads_philosopher_t* r = &ps.philosopher[(i + 1) % n];
        p->right_fork = r->left_fork;
        p->thread = threads.start(threads_philosopher_routine, p);
    }
    // wait for all philosophers being fed up:
    for (int32_t i = 0; i < n; i++) { events.wait(ps.fed_up[i]); }
    ps.enough = true;
    // join all philosopher threads
    for (int32_t i = 0; i < n; i++) {
        threads_philosopher_t* p = &ps.philosopher[i];
        threads.join(p->thread, -1);
    }
    // Dispose of mutexes and events
    for (int32_t i = 0; i < n; ++i) {
        threads_philosopher_t* p = &ps.philosopher[i];
        mutexes.dispose(&p->fork);
        events.dispose(ps.fed_up[i]);
    }
    // detached threads are hacky and not that swell of an idea
    // but sometimes can be useful for 1. quick hacks 2. threads
    // that execute blocking calls that e.g. write logs to the
    // internet service that hangs.
    // test detached threads
    thread_t detached_sleep = threads.start(threads_detached_sleep, null);
    threads.detach(detached_sleep);
    thread_t detached_loop = threads.start(threads_detached_loop, null);
    threads.detach(detached_loop);
    // leave detached threads sleeping and running till ExitProcess(0)
    // that should NOT hang.
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#pragma pop_macro("verbose")

#else
static void threads_test(void) { }
#endif

threads_if threads = {
    .start     = threads_start,
    .join      = threads_join,
    .detach    = threads_detach,
    .name      = threads_name,
    .realtime  = threads_realtime,
    .yield     = threads_yield,
    .sleep_for = threads_sleep_for,
    .id        = threads_id,
    .test      = threads_test
};
