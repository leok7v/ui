#pragma once
#include "ut/std.h"

begin_c

typedef struct { byte content[40]; } mutex_t;

typedef struct {
    void (*init)(mutex_t* m);
    void (*lock)(mutex_t* m);
    void (*unlock)(mutex_t* m);
    void (*dispose)(mutex_t* m);
    void (*test)(void);
} mutex_if;

extern mutex_if mutexes;

end_c

