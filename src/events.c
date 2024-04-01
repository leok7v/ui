#include "runtime.h"
#include "win32.h"

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

static int events_wait_or_timeout(event_t e, double seconds) {
    uint32_t ms = seconds < 0 ? INFINITE : (int32_t)(seconds * 1000.0 + 0.5);
    uint32_t r = 0;
    fatal_if_false((r = WaitForSingleObject(e, ms)) != WAIT_FAILED);
    return r == WAIT_OBJECT_0 ? 0 : -1; // all WAIT_ABANDONED as -1
}

static void events_wait(event_t e) { events_wait_or_timeout(e, -1); }

static int events_wait_any_or_timeout(int n, event_t events_[], double s) {
    uint32_t ms = s < 0 ? INFINITE : (int32_t)(s * 1000.0 + 0.5);
    uint32_t r = 0;
    fatal_if_false((r = WaitForMultipleObjects(n, events_, false, ms)) != WAIT_FAILED);
    // all WAIT_ABANDONED_0 and WAIT_IO_COMPLETION 0xC0 as -1
    return r < WAIT_OBJECT_0 + n ? r - WAIT_OBJECT_0 : -1;
}

static int events_wait_any(int n, event_t e[]) {
    return events_wait_any_or_timeout(n, e, -1);
}

static void events_dispose(event_t handle) {
    fatal_if_false(CloseHandle(handle));
}

// test:

static int32_t events_test_verbosity;

// check if the elapsed time is within the expected range
static void events_test_check_time(double start, double expected) {
    double elapsed = clock.seconds() - start;
    // Old Windows scheduler is prone to 2x16.6ms ~ 33ms delays
    swear(elapsed >= expected - 0.04 && elapsed <= expected + 0.04,
          "expected: %f elapsed %f seconds", expected, elapsed);
}

static void events_test(int32_t verbosity) {
#ifdef RUNTIME_TESTS
    events_test_verbosity = verbosity;
    event_t event = events.create();
    double start = clock.seconds();
    events.set(event);
    events.wait(event);
    events_test_check_time(start, 0); // Event should be immediate
    events.reset(event);
    start = clock.seconds();
    const double timeout_seconds = 0.01;
    int result = events.wait_or_timeout(event, timeout_seconds);
    events_test_check_time(start, timeout_seconds);
    swear(result == -1); // Timeout expected
    enum { count = 5 };
    event_t event_array[count];
    for (int i = 0; i < countof(event_array); i++) {
        event_array[i] = events.create_manual();
    }
    start = clock.seconds();
    events.set(event_array[2]); // Set the third event
    int index = events.wait_any(countof(event_array), event_array);
    events_test_check_time(start, 0);
    swear(index == 2); // Third event should be triggered
    events.reset(event_array[2]); // Reset the third event
    start = clock.seconds();
    result = events.wait_any_or_timeout(countof(event_array),
        event_array, timeout_seconds);
    events_test_check_time(start, timeout_seconds);
    swear(result == -1); // Timeout expected
    // Clean up
    events.dispose(event);
    for (int i = 0; i < countof(event_array); i++) {
        events.dispose(event_array[i]);
    }
    if (verbosity > 0) { traceln("done"); }
#else
    (void)unused(verbosity);
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
