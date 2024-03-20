#include "rt.h"

static_assertion(sizeof(CRITICAL_SECTION) == sizeof(mutex_t));

static void mutex_init(mutex_t* m) {
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)m;
    fatal_if_false(
        InitializeCriticalSectionAndSpinCount(cs, 4096)
    );
}

static void mutex_lock(mutex_t* m) { EnterCriticalSection((CRITICAL_SECTION*)m); }

static void mutex_unlock(mutex_t* m) { LeaveCriticalSection((CRITICAL_SECTION*)m); }

static void mutex_dispose(mutex_t* m) { DeleteCriticalSection((CRITICAL_SECTION*)m); }

mutex_if mutexes = {
    .init = mutex_init,
    .lock = mutex_lock,
    .unlock = mutex_unlock,
    .dispose = mutex_dispose
};

