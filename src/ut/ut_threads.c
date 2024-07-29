#include "ut/ut.h"
#include "ut/ut_win32.h"

// events:

static rt_event_t rt_event_create(void) {
    HANDLE e = CreateEvent(null, false, false, null);
    rt_not_null(e);
    return (rt_event_t)e;
}

static rt_event_t rt_event_create_manual(void) {
    HANDLE e = CreateEvent(null, true, false, null);
    rt_not_null(e);
    return (rt_event_t)e;
}

static void rt_event_set(rt_event_t e) {
    rt_fatal_win32err(SetEvent((HANDLE)e));
}

static void rt_event_reset(rt_event_t e) {
    rt_fatal_win32err(ResetEvent((HANDLE)e));
}

static int32_t rt_event_wait_or_timeout(rt_event_t e, fp64_t seconds) {
    uint32_t ms = seconds < 0 ? INFINITE : (uint32_t)(seconds * 1000.0 + 0.5);
    DWORD i = WaitForSingleObject(e, ms);
    rt_swear(i != WAIT_FAILED, "i: %d", i);
    errno_t r = rt_wait_ix2e(i);
    if (r != 0) { rt_swear(i == WAIT_TIMEOUT || i == WAIT_ABANDONED); }
    return i == WAIT_TIMEOUT ? -1 : (i == WAIT_ABANDONED ? -2 : i);
}

static void rt_event_wait(rt_event_t e) { rt_event_wait_or_timeout(e, -1); }

static int32_t rt_event_wait_any_or_timeout(int32_t n,
        rt_event_t events[], fp64_t s) {
    rt_swear(n < 64); // Win32 API limit
    const uint32_t ms = s < 0 ? INFINITE : (uint32_t)(s * 1000.0 + 0.5);
    const HANDLE* es = (const HANDLE*)events;
    DWORD i = WaitForMultipleObjects((DWORD)n, es, false, ms);
    rt_swear(i != WAIT_FAILED, "i: %d", i);
    errno_t r = rt_wait_ix2e(i);
    if (r != 0) { rt_swear(i == WAIT_TIMEOUT || i == WAIT_ABANDONED); }
    return i == WAIT_TIMEOUT ? -1 : (i == WAIT_ABANDONED ? -2 : i);
}

static int32_t rt_event_wait_any(int32_t n, rt_event_t e[]) {
    return rt_event_wait_any_or_timeout(n, e, -1);
}

static void rt_event_dispose(rt_event_t h) {
    rt_win32_close_handle(h);
}

#ifdef UT_TESTS

// test:

// check if the elapsed time is within the expected range
static void rt_event_test_check_time(fp64_t start, fp64_t expected) {
    fp64_t elapsed = rt_clock.seconds() - start;
    // Old Windows scheduler is prone to 2x16.6ms ~ 33ms delays (observed)
    rt_swear(elapsed >= expected - 0.04 && elapsed <= expected + 0.250,
          "expected: %f elapsed %f seconds", expected, elapsed);
}

static void rt_event_test(void) {
    rt_event_t event = rt_event.create();
    fp64_t start = rt_clock.seconds();
    rt_event.set(event);
    rt_event.wait(event);
    rt_event_test_check_time(start, 0); // Event should be immediate
    rt_event.reset(event);
    start = rt_clock.seconds();
    const fp64_t timeout_seconds = 1.0 / 8.0;
    int32_t result = rt_event.wait_or_timeout(event, timeout_seconds);
    rt_event_test_check_time(start, timeout_seconds);
    rt_swear(result == -1); // Timeout expected
    enum { count = 5 };
    rt_event_t events[count];
    for (int32_t i = 0; i < rt_countof(events); i++) {
        events[i] = rt_event.create_manual();
    }
    start = rt_clock.seconds();
    rt_event.set(events[2]); // Set the third event
    int32_t index = rt_event.wait_any(rt_countof(events), events);
    rt_swear(index == 2);
    rt_event_test_check_time(start, 0);
    rt_swear(index == 2); // Third event should be triggered
    rt_event.reset(events[2]); // Reset the third event
    start = rt_clock.seconds();
    result = rt_event.wait_any_or_timeout(rt_countof(events), events, timeout_seconds);
    rt_swear(result == -1);
    rt_event_test_check_time(start, timeout_seconds);
    rt_swear(result == -1); // Timeout expected
    // Clean up
    rt_event.dispose(event);
    for (int32_t i = 0; i < rt_countof(events); i++) {
        rt_event.dispose(events[i]);
    }
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#else

static void rt_event_test(void) { }

#endif

rt_event_if rt_event = {
    .create              = rt_event_create,
    .create_manual       = rt_event_create_manual,
    .set                 = rt_event_set,
    .reset               = rt_event_reset,
    .wait                = rt_event_wait,
    .wait_or_timeout     = rt_event_wait_or_timeout,
    .wait_any            = rt_event_wait_any,
    .wait_any_or_timeout = rt_event_wait_any_or_timeout,
    .dispose             = rt_event_dispose,
    .test                = rt_event_test
};

// mutexes:

rt_static_assertion(sizeof(CRITICAL_SECTION) == sizeof(rt_mutex_t));

static void rt_mutex_init(rt_mutex_t* m) {
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)m;
    rt_fatal_win32err(InitializeCriticalSectionAndSpinCount(cs, 4096));
}

static void rt_mutex_lock(rt_mutex_t* m) {
    EnterCriticalSection((CRITICAL_SECTION*)m);
}

static void rt_mutex_unlock(rt_mutex_t* m) {
    LeaveCriticalSection((CRITICAL_SECTION*)m);
}

static void rt_mutex_dispose(rt_mutex_t* m) {
    DeleteCriticalSection((CRITICAL_SECTION*)m);
}

// test:

// check if the elapsed time is within the expected range
static void rt_mutex_test_check_time(fp64_t start, fp64_t expected) {
    fp64_t elapsed = rt_clock.seconds() - start;
    // Old Windows scheduler is prone to 2x16.6ms ~ 33ms delays
    rt_swear(elapsed >= expected - 0.04 && elapsed <= expected + 0.04,
          "expected: %f elapsed %f seconds", expected, elapsed);
}

static void rt_mutex_test_lock_unlock(void* arg) {
    rt_mutex_t* mutex = (rt_mutex_t*)arg;
    rt_mutex.lock(mutex);
    rt_thread.sleep_for(0.01); // Hold the mutex for 10ms
    rt_mutex.unlock(mutex);
}

static void rt_mutex_test(void) {
    rt_mutex_t mutex;
    rt_mutex.init(&mutex);
    fp64_t start = rt_clock.seconds();
    rt_mutex.lock(&mutex);
    rt_mutex.unlock(&mutex);
    // Lock and unlock should be immediate
    rt_mutex_test_check_time(start, 0);
    enum { count = 5 };
    rt_thread_t ts[count];
    for (int32_t i = 0; i < rt_countof(ts); i++) {
        ts[i] = rt_thread.start(rt_mutex_test_lock_unlock, &mutex);
    }
    // Wait for all threads to finish
    for (int32_t i = 0; i < rt_countof(ts); i++) {
        rt_thread.join(ts[i], -1);
    }
    rt_mutex.dispose(&mutex);
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

rt_mutex_if rt_mutex = {
    .init    = rt_mutex_init,
    .lock    = rt_mutex_lock,
    .unlock  = rt_mutex_unlock,
    .dispose = rt_mutex_dispose,
    .test    = rt_mutex_test
};

// threads:

static void* rt_thread_ntdll(void) {
    static HMODULE ntdll;
    if (ntdll == null) {
        ntdll = (void*)GetModuleHandleA("ntdll.dll");
    }
    if (ntdll == null) {
        ntdll = rt_loader.open("ntdll.dll", 0);
    }
    rt_not_null(ntdll);
    return ntdll;
}

static fp64_t rt_thread_ns2ms(int64_t ns) {
    return (fp64_t)ns / (fp64_t)rt_clock.nsec_in_msec;
}

static void rt_thread_set_timer_resolution(uint64_t nanoseconds) {
    typedef int32_t (*query_timer_resolution_t)(ULONG* minimum_resolution,
        ULONG* maximum_resolution, ULONG* actual_resolution);
    typedef int32_t (*set_timer_resolution_t)(ULONG requested_resolution,
        BOOLEAN set, ULONG* actual_resolution); // ntdll.dll
    void* nt_dll = rt_thread_ntdll();
    query_timer_resolution_t query_timer_resolution =  (query_timer_resolution_t)
        rt_loader.sym(nt_dll, "NtQueryTimerResolution");
    set_timer_resolution_t set_timer_resolution = (set_timer_resolution_t)
        rt_loader.sym(nt_dll, "NtSetTimerResolution");
    unsigned long min100ns = 16 * 10 * 1000;
    unsigned long max100ns =  1 * 10 * 1000;
    unsigned long cur100ns =  0;
    rt_fatal_if(query_timer_resolution(&min100ns, &max100ns, &cur100ns) != 0);
    uint64_t max_ns = max100ns * 100uLL;
//  uint64_t min_ns = min100ns * 100uLL;
//  uint64_t cur_ns = cur100ns * 100uLL;
    // max resolution is lowest possible delay between timer events
//  if (rt_debug.verbosity.level >= rt_debug.verbosity.trace) {
//      rt_println("timer resolution min: %.3f max: %.3f cur: %.3f"
//          " ms (milliseconds)",
//          rt_thread_ns2ms(min_ns),
//          rt_thread_ns2ms(max_ns),
//          rt_thread_ns2ms(cur_ns));
//  }
    // note that maximum resolution is actually < minimum
    nanoseconds = rt_max(max_ns, nanoseconds);
    unsigned long ns = (unsigned long)((nanoseconds + 99) / 100);
    rt_fatal_if(set_timer_resolution(ns, true, &cur100ns) != 0);
    rt_fatal_if(query_timer_resolution(&min100ns, &max100ns, &cur100ns) != 0);
//  if (rt_debug.verbosity.level >= rt_debug.verbosity.trace) {
//      min_ns = min100ns * 100uLL;
//      max_ns = max100ns * 100uLL; // the smallest interval
//      cur_ns = cur100ns * 100uLL;
//      rt_println("timer resolution min: %.3f max: %.3f cur: %.3f ms (milliseconds)",
//          rt_thread_ns2ms(min_ns),
//          rt_thread_ns2ms(max_ns),
//          rt_thread_ns2ms(cur_ns));
//  }
}

static void rt_thread_power_throttling_disable_for_process(void) {
    static bool disabled_for_the_process;
    if (!disabled_for_the_process) {
        PROCESS_POWER_THROTTLING_STATE pt = { 0 };
        pt.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
        pt.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
        pt.StateMask = 0;
        rt_fatal_win32err(SetProcessInformation(GetCurrentProcess(),
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

static void rt_thread_power_throttling_disable_for_thread(HANDLE thread) {
    THREAD_POWER_THROTTLING_STATE pt = { 0 };
    pt.Version = THREAD_POWER_THROTTLING_CURRENT_VERSION;
    pt.ControlMask = THREAD_POWER_THROTTLING_EXECUTION_SPEED;
    pt.StateMask = 0;
    rt_fatal_win32err(SetThreadInformation(thread,
        ThreadPowerThrottling, &pt, sizeof(pt)));
}

static void rt_thread_disable_power_throttling(void) {
    rt_thread_power_throttling_disable_for_process();
    rt_thread_power_throttling_disable_for_thread(GetCurrentThread());
}

static const char* rt_thread_rel2str(int32_t rel) {
    switch (rel) {
        case RelationProcessorCore   : return "ProcessorCore   ";
        case RelationNumaNode        : return "NumaNode        ";
        case RelationCache           : return "Cache           ";
        case RelationProcessorPackage: return "ProcessorPackage";
        case RelationGroup           : return "Group           ";
        case RelationProcessorDie    : return "ProcessorDie    ";
        case RelationNumaNodeEx      : return "NumaNodeEx      ";
        case RelationProcessorModule : return "ProcessorModule ";
        default: rt_assert(false, "fix me"); return "???";
    }
}

static uint64_t rt_thread_next_physical_processor_affinity_mask(void) {
    static volatile int32_t initialized;
    static int32_t init;
    static int32_t next = 1; // next physical core to use
    static int32_t cores = 0; // number of physical processors (cores)
    static uint64_t any;
    static uint64_t affinity[64]; // mask for each physical processor
    bool set_to_true = rt_atomics.compare_exchange_int32(&init, false, true);
    if (set_to_true) {
        // Concept D: 6 cores, 12 logical processors: 27 lpi entries
        static SYSTEM_LOGICAL_PROCESSOR_INFORMATION lpi[64];
        DWORD bytes = 0;
        GetLogicalProcessorInformation(null, &bytes);
        rt_assert(bytes % sizeof(lpi[0]) == 0);
        // number of lpi entries == 27 on 6 core / 12 logical processors system
        int32_t n = bytes / sizeof(lpi[0]);
        rt_assert(bytes <= sizeof(lpi), "increase lpi[%d]", n);
        rt_fatal_win32err(GetLogicalProcessorInformation(&lpi[0], &bytes));
        for (int32_t i = 0; i < n; i++) {
//          if (rt_debug.verbosity.level >= rt_debug.verbosity.trace) {
//              rt_println("[%2d] affinity mask 0x%016llX relationship=%d %s", i,
//                  lpi[i].ProcessorMask, lpi[i].Relationship,
//                  rt_thread_rel2str(lpi[i].Relationship));
//          }
            if (lpi[i].Relationship == RelationProcessorCore) {
                rt_assert(cores < rt_countof(affinity), "increase affinity[%d]", cores);
                if (cores < rt_countof(affinity)) {
                    any |= lpi[i].ProcessorMask;
                    affinity[cores] = lpi[i].ProcessorMask;
                    cores++;
                }
            }
        }
        initialized = true;
    } else {
        while (initialized == 0) { rt_thread.sleep_for(1 / 1024.0); }
        rt_assert(any != 0); // should not ever happen
        if (any == 0) { any = (uint64_t)(-1LL); }
    }
    uint64_t mask = next < cores ? affinity[next] : any;
    rt_assert(mask != 0);
    // assume last physical core is least popular
    if (next < cores) { next++; } // not circular
    return mask;
}

static void rt_thread_realtime(void) {
    rt_fatal_win32err(SetPriorityClass(GetCurrentProcess(),
        REALTIME_PRIORITY_CLASS));
    rt_fatal_win32err(SetThreadPriority(GetCurrentThread(),
        THREAD_PRIORITY_TIME_CRITICAL));
    rt_fatal_win32err(SetThreadPriorityBoost(GetCurrentThread(),
        /* bDisablePriorityBoost = */ false));
    // desired: 0.5ms = 500us (microsecond) = 50,000ns
    rt_thread_set_timer_resolution((uint64_t)rt_clock.nsec_in_usec * 500ULL);
    rt_fatal_win32err(SetThreadAffinityMask(GetCurrentThread(),
        rt_thread_next_physical_processor_affinity_mask()));
    rt_thread_disable_power_throttling();
}

static void rt_thread_yield(void) { SwitchToThread(); }

static rt_thread_t rt_thread_start(void (*func)(void*), void* p) {
    rt_thread_t t = (rt_thread_t)CreateThread(null, 0,
        (LPTHREAD_START_ROUTINE)(void*)func, p, 0, null);
    rt_not_null(t);
    return t;
}

static bool is_handle_valid(void* h) {
    DWORD flags = 0;
    return GetHandleInformation(h, &flags);
}

static errno_t rt_thread_join(rt_thread_t t, fp64_t timeout) {
    rt_not_null(t);
    rt_fatal_if(!is_handle_valid(t));
    const uint32_t ms = timeout < 0 ? INFINITE : (uint32_t)(timeout * 1000.0 + 0.5);
    DWORD ix = WaitForSingleObject(t, (DWORD)ms);
    errno_t r = rt_wait_ix2e(ix);
    rt_assert(r != ERROR_REQUEST_ABORTED, "AFAIK thread can`t be ABANDONED");
    if (r == 0) {
        rt_win32_close_handle(t);
    } else {
        rt_println("failed to join thread %p %s", t, rt_strerr(r));
    }
    return r;
}

static void rt_thread_detach(rt_thread_t t) {
    rt_not_null(t);
    rt_fatal_if(!is_handle_valid(t));
    rt_win32_close_handle(t);
}

static void rt_thread_name(const char* name) {
    uint16_t stack[128];
    rt_fatal_if(rt_str.len(name) >= rt_countof(stack), "name too long: %s", name);
    rt_str.utf8to16(stack, rt_countof(stack), name, -1);
    HRESULT r = SetThreadDescription(GetCurrentThread(), stack);
    // notoriously returns 0x10000000 for no good reason whatsoever
    rt_fatal_if(!SUCCEEDED(r));
}

static void rt_thread_sleep_for(fp64_t seconds) {
    rt_assert(seconds >= 0);
    if (seconds < 0) { seconds = 0; }
    int64_t ns100 = (int64_t)(seconds * 1.0e+7); // in 0.1 us aka 100ns
    typedef int32_t (__stdcall *nt_delay_execution_t)(BOOLEAN alertable,
        PLARGE_INTEGER DelayInterval);
    static nt_delay_execution_t NtDelayExecution;
    // delay in 100-ns units. negative value means delay relative to current.
    LARGE_INTEGER delay = {0}; // delay in 100-ns units.
    delay.QuadPart = -ns100; // negative value means delay relative to current.
    if (NtDelayExecution == null) {
        void* ntdll = rt_thread_ntdll();
        NtDelayExecution = (nt_delay_execution_t)
            rt_loader.sym(ntdll, "NtDelayExecution");
        rt_not_null(NtDelayExecution);
    }
    // If "alertable" is set, sleep_for() can break earlier
    // as a result of NtAlertThread call.
    NtDelayExecution(false, &delay);
}

static uint64_t rt_thread_id_of(rt_thread_t t) {
    return (uint64_t)GetThreadId((HANDLE)t);
}

static uint64_t rt_thread_id(void) {
    return (uint64_t)GetThreadId(GetCurrentThread());
}

static rt_thread_t rt_thread_self(void) {
    // GetCurrentThread() returns pseudo-handle, not a real handle
    // if real handle is ever needed may do
    // rt_thread_t t = rt_thread.open(rt_thread.id()) and
    // rt_thread.close(t) instead.
    return (rt_thread_t)GetCurrentThread();
}

static errno_t rt_thread_open(rt_thread_t *t, uint64_t id) {
    // GetCurrentThread() returns pseudo-handle, not a real handle.
    // if real handle is ever needed do rt_thread_id_of() instead
    // but don't forget to do rt_thread.close() after that.
    *t = (rt_thread_t)OpenThread(THREAD_ALL_ACCESS, false, (DWORD)id);
    return *t == null ? rt_core.err() : 0;
}

static void rt_thread_close(rt_thread_t t) {
    rt_not_null(t);
    rt_win32_close_handle((HANDLE)t);
}

#ifdef UT_TESTS

// test: https://en.wikipedia.org/wiki/Dining_philosophers_problem

typedef struct rt_thread_philosophers_s rt_thread_philosophers_t;

typedef struct {
    rt_thread_philosophers_t* ps;
    rt_mutex_t  fork;
    rt_mutex_t* left_fork;
    rt_mutex_t* right_fork;
    rt_thread_t thread;
    uint64_t    id;
} rt_thread_philosopher_t;

typedef struct rt_thread_philosophers_s {
    rt_thread_philosopher_t philosopher[3];
    rt_event_t fed_up[3];
    uint32_t seed;
    volatile bool enough;
} rt_thread_philosophers_t;

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                 \
    if (rt_debug.verbosity.level >= rt_debug.verbosity.trace) { \
        rt_println(__VA_ARGS__);                             \
    }                                                     \
} while (0)

static void rt_thread_philosopher_think(rt_thread_philosopher_t* p) {
    verbose("philosopher %d is thinking.", p->id);
    // Random think time between .1 and .3 seconds
    fp64_t seconds = (rt_num.random32(&p->ps->seed) % 30 + 1) / 100.0;
    rt_thread.sleep_for(seconds);
}

static void rt_thread_philosopher_eat(rt_thread_philosopher_t* p) {
    verbose("philosopher %d is eating.", p->id);
    // Random eat time between .1 and .2 seconds
    fp64_t seconds = (rt_num.random32(&p->ps->seed) % 20 + 1) / 100.0;
    rt_thread.sleep_for(seconds);
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

static void rt_thread_philosopher_routine(void* arg) {
    rt_thread_philosopher_t* p = (rt_thread_philosopher_t*)arg;
    enum { n = rt_countof(p->ps->philosopher) };
    rt_thread.name("philosopher");
    rt_thread.realtime();
    while (!p->ps->enough) {
        rt_thread_philosopher_think(p);
        if (p->id == n - 1) { // Last philosopher picks up the right fork first
            rt_mutex.lock(p->right_fork);
            verbose("philosopher %d picked up right fork.", p->id);
            rt_mutex.lock(p->left_fork);
            verbose("philosopher %d picked up left fork.", p->id);
        } else { // Other philosophers pick up the left fork first
            rt_mutex.lock(p->left_fork);
            verbose("philosopher %d picked up left fork.", p->id);
            rt_mutex.lock(p->right_fork);
            verbose("philosopher %d picked up right fork.", p->id);
        }
        rt_thread_philosopher_eat(p);
        rt_mutex.unlock(p->right_fork);
        verbose("philosopher %d put down right fork.", p->id);
        rt_mutex.unlock(p->left_fork);
        verbose("philosopher %d put down left fork.", p->id);
        rt_event.set(p->ps->fed_up[p->id]);
    }
}

static void rt_thread_detached_sleep(void* rt_unused(p)) {
    rt_thread.sleep_for(1000.0); // seconds
}

static void rt_thread_detached_loop(void* rt_unused(p)) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i < UINT64_MAX; i++) { sum += i; }
    // make sure that compiler won't get rid of the loop:
    rt_swear(sum == 0x8000000000000001ULL, "sum: %llu 0x%16llX", sum, sum);
}

static void rt_thread_test(void) {
    rt_thread_philosophers_t ps = { .seed = 1 };
    enum { n = rt_countof(ps.philosopher) };
    // Initialize mutexes for forks
    for (int32_t i = 0; i < n; i++) {
        rt_thread_philosopher_t* p = &ps.philosopher[i];
        p->id = i;
        p->ps = &ps;
        rt_mutex.init(&p->fork);
        p->left_fork = &p->fork;
        ps.fed_up[i] = rt_event.create();
    }
    // Create and start philosopher threads
    for (int32_t i = 0; i < n; i++) {
        rt_thread_philosopher_t* p = &ps.philosopher[i];
        rt_thread_philosopher_t* r = &ps.philosopher[(i + 1) % n];
        p->right_fork = r->left_fork;
        p->thread = rt_thread.start(rt_thread_philosopher_routine, p);
    }
    // wait for all philosophers being fed up:
    for (int32_t i = 0; i < n; i++) { rt_event.wait(ps.fed_up[i]); }
    ps.enough = true;
    // join all philosopher threads
    for (int32_t i = 0; i < n; i++) {
        rt_thread_philosopher_t* p = &ps.philosopher[i];
        rt_thread.join(p->thread, -1);
    }
    // Dispose of mutexes and events
    for (int32_t i = 0; i < n; ++i) {
        rt_thread_philosopher_t* p = &ps.philosopher[i];
        rt_mutex.dispose(&p->fork);
        rt_event.dispose(ps.fed_up[i]);
    }
    // detached threads are hacky and not that swell of an idea
    // but sometimes can be useful for 1. quick hacks 2. threads
    // that execute blocking calls that e.g. write logs to the
    // internet service that hangs.
    // test detached threads
    rt_thread_t detached_sleep = rt_thread.start(rt_thread_detached_sleep, null);
    rt_thread.detach(detached_sleep);
    rt_thread_t detached_loop = rt_thread.start(rt_thread_detached_loop, null);
    rt_thread.detach(detached_loop);
    // leave detached threads sleeping and running till ExitProcess(0)
    // that should NOT hang.
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#pragma pop_macro("verbose")

#else
static void rt_thread_test(void) { }
#endif

rt_thread_if rt_thread = {
    .start     = rt_thread_start,
    .join      = rt_thread_join,
    .detach    = rt_thread_detach,
    .name      = rt_thread_name,
    .realtime  = rt_thread_realtime,
    .yield     = rt_thread_yield,
    .sleep_for = rt_thread_sleep_for,
    .id_of     = rt_thread_id_of,
    .id        = rt_thread_id,
    .self      = rt_thread_self,
    .open      = rt_thread_open,
    .close     = rt_thread_close,
    .test      = rt_thread_test
};
