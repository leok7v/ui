#include "rt.h"
#include "win32.h"

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

static void mutexes_test(int32_t verbosity) {
    // TODO: implement me
    if (verbosity > 0) { traceln("done"); }
}

mutex_if mutexes = {
    .init    = mutexes_init,
    .lock    = mutexes_lock,
    .unlock  = mutexes_unlock,
    .dispose = mutexes_dispose,
    .test    = mutexes_test
};

