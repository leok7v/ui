#pragma once
#include "util/manifest.h"

begin_c

typedef void* thread_t;

typedef struct {
    thread_t (*start)(void (*func)(void*), void* p); // never returns null
    bool (*try_join)(thread_t thread, double timeout); // seconds
    void (*join)(thread_t thread);
    void (*name)(const char* name); // names the thread
    void (*realtime)(void); // bumps calling thread priority
    void (*yield)(void);    // pthread_yield() / Win32: SwitchToThread()
    void (*sleep_for)(double seconds);
    int32_t (*id)(void);    // gettid()
    void (*test)(void);
} threads_if;

extern threads_if threads;

end_c
