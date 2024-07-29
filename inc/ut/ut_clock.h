#pragma once
#include "ut/ut_std.h"

ut_begin_c

typedef struct {
    int32_t const nsec_in_usec; // nano in micro second
    int32_t const nsec_in_msec; // nano in milli
    int32_t const nsec_in_sec;
    int32_t const usec_in_msec; // micro in milli
    int32_t const msec_in_sec;  // milli in sec
    int32_t const usec_in_sec;  // micro in sec
    fp64_t   (*seconds)(void);      // since boot
    uint64_t (*nanoseconds)(void);  // since boot overflows in about 584.5 years
    uint64_t (*unix_microseconds)(void); // since January 1, 1970
    uint64_t (*unix_seconds)(void);      // since January 1, 1970
    uint64_t (*microseconds)(void); // NOT monotonic(!) UTC since epoch January 1, 1601
    uint64_t (*localtime)(void);    // local time microseconds since epoch
    void (*utc)(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms,
        int32_t* mc);
    void (*local)(uint64_t microseconds, int32_t* year, int32_t* month,
        int32_t* day, int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms,
        int32_t* mc);
    void (*test)(void);
} rt_clock_if;

extern rt_clock_if rt_clock;

ut_end_c

