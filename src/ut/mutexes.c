#include "ut/runtime.h"
#include "ut/win32.h"

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
static void mutexes_test_check_time(double start, double expected) {
    double elapsed = clock.seconds() - start;
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
    double start = clock.seconds();
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
        threads.join(ts[i]);
    }
    mutexes.dispose(&mutex);
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
}

mutex_if mutexes = {
    .init    = mutexes_init,
    .lock    = mutexes_lock,
    .unlock  = mutexes_unlock,
    .dispose = mutexes_dispose,
    .test    = mutexes_test
};
