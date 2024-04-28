#pragma once
#include "ut/std.h"

begin_c

typedef struct event_s * event_t;

typedef struct {
    event_t (*create)(void); // never returns null
    event_t (*create_manual)(void); // never returns null
    void (*set)(event_t e);
    void (*reset)(event_t e);
    void (*wait)(event_t e);
    // returns 0 or -1 on timeout
    int32_t (*wait_or_timeout)(event_t e, fp64_t seconds); // seconds < 0 forever
    // returns event index or -1 on timeout or abandon
    int32_t (*wait_any)(int32_t n, event_t events[]); // -1 on abandon
    int32_t (*wait_any_or_timeout)(int32_t n, event_t e[], fp64_t seconds);
    void (*dispose)(event_t e);
    void (*test)(void);
} events_if;

extern events_if events;

typedef struct { uint8_t content[40]; } mutex_t;

typedef struct {
    void (*init)(mutex_t* m);
    void (*lock)(mutex_t* m);
    void (*unlock)(mutex_t* m);
    void (*dispose)(mutex_t* m);
    void (*test)(void);
} mutex_if;

extern mutex_if mutexes;

typedef struct thread_s * thread_t;

typedef struct {
    thread_t (*start)(void (*func)(void*), void* p); // never returns null
    errno_t (*join)(thread_t thread, fp64_t timeout_seconds); // < 0 forever
    void (*detach)(thread_t thread); // closes handle. thread is not joinable
    void (*name)(const char* name); // names the thread
    void (*realtime)(void); // bumps calling thread priority
    void (*yield)(void);    // pthread_yield() / Win32: SwitchToThread()
    void (*sleep_for)(fp64_t seconds);
    int32_t (*id)(void);    // gettid()
    void (*test)(void);
} threads_if;

extern threads_if threads;

end_c
