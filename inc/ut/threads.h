#pragma once
#include "ut/std.h"

begin_c

typedef struct thread_s * thread_t;

typedef struct {
    thread_t (*start)(void (*func)(void*), void* p); // never returns null
    errno_t (*join)(thread_t thread, double timeout_seconds); // < 0 forever
    void (*detach)(thread_t thread); // closes handle. thread is not joinable
    void (*name)(const char* name); // names the thread
    void (*realtime)(void); // bumps calling thread priority
    void (*yield)(void);    // pthread_yield() / Win32: SwitchToThread()
    void (*sleep_for)(double seconds);
    int32_t (*id)(void);    // gettid()
    void (*test)(void);
} threads_if;

extern threads_if threads;

end_c
