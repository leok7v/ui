#include "rt.h"

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

events_if events = {
    events_create,
    events_create_manual,
    events_set,
    events_reset,
    events_wait,
    events_wait_or_timeout,
    events_wait_any,
    events_wait_any_or_timeout,
    events_dispose
};
