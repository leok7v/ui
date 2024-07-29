#pragma once
#include "rt/rt_std.h"

rt_begin_c

typedef struct rt_event_s* rt_event_t;

typedef struct {
    rt_event_t (*create)(void); // never returns null
    rt_event_t (*create_manual)(void); // never returns null
    void (*set)(rt_event_t e);
    void (*reset)(rt_event_t e);
    void (*wait)(rt_event_t e);
    // returns 0 on success or -1 on timeout
    int32_t (*wait_or_timeout)(rt_event_t e, fp64_t seconds); // seconds < 0 forever
    // returns event index or -1 on timeout or -2 on abandon
    int32_t (*wait_any)(int32_t n, rt_event_t events[]); // -1 on abandon
    int32_t (*wait_any_or_timeout)(int32_t n, rt_event_t e[], fp64_t seconds);
    void (*dispose)(rt_event_t e);
    void (*test)(void);
} rt_event_if;

extern rt_event_if rt_event;

typedef struct rt_aligned_8 rt_mutex_s { uint8_t content[40]; } rt_mutex_t;

typedef struct {
    void (*init)(rt_mutex_t* m);
    void (*lock)(rt_mutex_t* m);
    void (*unlock)(rt_mutex_t* m);
    void (*dispose)(rt_mutex_t* m);
    void (*test)(void);
} rt_mutex_if;

extern rt_mutex_if rt_mutex;

typedef struct thread_s* rt_thread_t;

typedef struct {
    rt_thread_t (*start)(void (*func)(void*), void* p); // never returns null
    errno_t     (*join)(rt_thread_t thread, fp64_t timeout_seconds); // < 0 forever
    void        (*detach)(rt_thread_t thread); // closes handle. thread is not joinable
    void        (*name)(const char* name); // names the thread
    void        (*realtime)(void); // bumps calling thread priority
    void        (*yield)(void);    // pthread_yield() / Win32: SwitchToThread()
    void        (*sleep_for)(fp64_t seconds);
    uint64_t    (*id_of)(rt_thread_t t);
    uint64_t    (*id)(void); // gettid()
    rt_thread_t (*self)(void); // Pseudo Handle may differ in access to .open(.id())
    errno_t     (*open)(rt_thread_t* t, uint64_t id);
    void        (*close)(rt_thread_t t);
    void        (*test)(void);
} rt_thread_if;

extern rt_thread_if rt_thread;

rt_end_c
