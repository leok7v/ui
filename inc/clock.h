#pragma once
#include "manifest.h"

begin_c

enum {
    nsec_in_usec = 1000,
    nsec_in_msec = nsec_in_usec * 1000,
    nsec_in_sec  = nsec_in_msec * 1000,
    msec_in_sec  = 1000 * 1000
};

typedef struct {
    double   (*seconds)(void);      // since boot
    uint64_t (*nanoseconds)(void);  // since boot overflows in about 584.5 years
    uint64_t (*unix_microseconds)(void); // since January 1, 1970
    uint64_t (*unix_seconds)(void);      // since January 1, 1970
    uint64_t (*microseconds)(void); // NOT monotonic(!) UTC since epoch January 1, 1601
    uint64_t (*localtime)(void);    // local time microseconds since epoch
    void (*time_utc)(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc);
    void (*time_local)(uint64_t microseconds, int* year, int* month,
        int* day, int* hh, int* mm, int* ss, int* ms, int* mc);
    void (*test)(int32_t verbosity);
} clock_if;

extern clock_if clock;

end_c

