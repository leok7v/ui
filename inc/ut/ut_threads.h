#pragma once
#include "ut/ut_std.h"

begin_c

typedef struct ut_event_s* ut_event_t;

typedef struct {
    ut_event_t (*create)(void); // never returns null
    ut_event_t (*create_manual)(void); // never returns null
    void (*set)(ut_event_t e);
    void (*reset)(ut_event_t e);
    void (*wait)(ut_event_t e);
    // returns 0 on success or -1 on timeout
    int32_t (*wait_or_timeout)(ut_event_t e, fp64_t seconds); // seconds < 0 forever
    // returns event index or -1 on timeout or -2 on abandon
    int32_t (*wait_any)(int32_t n, ut_event_t events[]); // -1 on abandon
    int32_t (*wait_any_or_timeout)(int32_t n, ut_event_t e[], fp64_t seconds);
    void (*dispose)(ut_event_t e);
    void (*test)(void);
} ut_event_if;

extern ut_event_if ut_event;

typedef struct ut_alligned_8 mutex_s { uint8_t content[40]; } ut_mutex_t;

typedef struct {
    void (*init)(ut_mutex_t* m);
    void (*lock)(ut_mutex_t* m);
    void (*unlock)(ut_mutex_t* m);
    void (*dispose)(ut_mutex_t* m);
    void (*test)(void);
} ut_mutex_if;

extern ut_mutex_if ut_mutex;

typedef struct thread_s* ut_thread_t;

typedef struct {
    ut_thread_t (*start)(void (*func)(void*), void* p); // never returns null
    errno_t     (*join)(ut_thread_t thread, fp64_t timeout_seconds); // < 0 forever
    void        (*detach)(ut_thread_t thread); // closes handle. thread is not joinable
    void        (*name)(const char* name); // names the thread
    void        (*realtime)(void); // bumps calling thread priority
    void        (*yield)(void);    // pthread_yield() / Win32: SwitchToThread()
    void        (*sleep_for)(fp64_t seconds);
    uint64_t    (*id_of)(ut_thread_t t);
    uint64_t    (*id)(void); // gettid()
    ut_thread_t (*self)(void); // Pseudo Handle may differ in access to .open(.id())
    errno_t     (*open)(ut_thread_t* t, uint64_t id);
    void        (*close)(ut_thread_t t);
    void        (*test)(void);
} ut_thread_if;

extern ut_thread_if ut_thread;

end_c
