#pragma once
#include "manifest.h"

begin_c

typedef void* event_t;

typedef struct {
    event_t (*create)(void); // never returns null
    event_t (*create_manual)(void); // never returns null
    void (*set)(event_t e);
    void (*reset)(event_t e);
    void (*wait)(event_t e);
    // returns 0 or -1 on timeout
    int (*wait_or_timeout)(event_t e, double seconds); // seconds < 0 forever
    // returns event index or -1 on timeout or abandon
    int (*wait_any)(int n, event_t events[]); // -1 on abandon
    int (*wait_any_or_timeout)(int n, event_t e[], double seconds);
    void (*dispose)(event_t e);
    void (*test)(int32_t verbosity);
} events_if;

extern events_if events;

end_c
