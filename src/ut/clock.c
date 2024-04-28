#include "ut/ut.h"
#include "ut/win32.h"

enum {
    clock_nsec_in_usec = 1000, // nano in micro
    clock_nsec_in_msec = clock_nsec_in_usec * 1000, // nano in milli
    clock_nsec_in_sec  = clock_nsec_in_msec * 1000,
    clock_usec_in_msec = 1000, // micro in mill
    clock_msec_in_sec  = 1000, // milli in sec
    clock_usec_in_sec  = clock_usec_in_msec * clock_msec_in_sec // micro in sec
};

static uint64_t clock_microseconds_since_epoch(void) { // NOT monotonic
    FILETIME ft; // time in 100ns interval (tenth of microsecond)
    // since 12:00 A.M. January 1, 1601 Coordinated Universal Time (UTC)
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t microseconds =
        (((uint64_t)ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10;
    assert(microseconds > 0);
    return microseconds;
}

static uint64_t clock_localtime(void) {
    TIME_ZONE_INFORMATION tzi; // UTC = local time + bias
    GetTimeZoneInformation(&tzi);
    uint64_t bias = (uint64_t)tzi.Bias * 60LL * 1000 * 1000; // in microseconds
    return clock_microseconds_since_epoch() - bias;
}

static void clock_utc(uint64_t microseconds,
        int32_t* year, int32_t* month, int32_t* day,
        int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms, int32_t* mc) {
    uint64_t time_in_100ns = microseconds * 10;
    FILETIME mst = { (DWORD)(time_in_100ns & 0xFFFFFFFF),
                     (DWORD)(time_in_100ns >> 32) };
    SYSTEMTIME utc;
    FileTimeToSystemTime(&mst, &utc);
    *year = utc.wYear;
    *month = utc.wMonth;
    *day = utc.wDay;
    *hh = utc.wHour;
    *mm = utc.wMinute;
    *ss = utc.wSecond;
    *ms = utc.wMilliseconds;
    *mc = microseconds % 1000;
}

static void clock_local(uint64_t microseconds,
        int32_t* year, int32_t* month, int32_t* day,
        int32_t* hh, int32_t* mm, int32_t* ss, int32_t* ms, int32_t* mc) {
    uint64_t time_in_100ns = microseconds * 10;
    FILETIME mst = { (DWORD)(time_in_100ns & 0xFFFFFFFF), (DWORD)(time_in_100ns >> 32) };
    SYSTEMTIME utc;
    FileTimeToSystemTime(&mst, &utc);
    DYNAMIC_TIME_ZONE_INFORMATION tzi;
    GetDynamicTimeZoneInformation(&tzi);
    SYSTEMTIME lt = {0};
    SystemTimeToTzSpecificLocalTimeEx(&tzi, &utc, &lt);
    *year = lt.wYear;
    *month = lt.wMonth;
    *day = lt.wDay;
    *hh = lt.wHour;
    *mm = lt.wMinute;
    *ss = lt.wSecond;
    *ms = lt.wMilliseconds;
    *mc = microseconds % 1000;
}

static double clock_seconds(void) { // since_boot
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static double one_over_freq;
    if (one_over_freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        one_over_freq = 1.0 / frequency.QuadPart;
    }
    return (double)qpc.QuadPart * one_over_freq;
}

// Max duration in nanoseconds=2^64 - 1 nanoseconds
//                          2^64 - 1 ns        1 sec          1 min
// Max Duration in Hours =  ----------- x  ------------ x -------------
//                          10^9 ns / s    60 sec / min   60 min / hour
//
//                              1 hour
// Max Duration in Days =  ---------------
//                          24 hours / day
//
// it would take approximately 213,503 days (or about 584.5 years)
// for clock.nanoseconds() to overflow
//
// for divider = num.gcd32(nsec_in_sec, freq) below and 10MHz timer
// the actual duration is shorter because of (mul == 100)
//    (uint64_t)qpc.QuadPart * mul
// 64 bit overflow and is about 5.8 years.
//
// In a long running code like services is advisable to use
// clock.nanoseconds() to measure only deltas and pay close attention
// to the wrap around despite of 5 years monotony

static uint64_t clock_nanoseconds(void) {
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static uint32_t freq;
    static uint32_t mul = clock_nsec_in_sec;
    if (freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        assert(frequency.HighPart == 0);
        // even 1GHz frequency should fit into 32 bit unsigned
        assert(frequency.HighPart == 0, "%08lX%%08lX",
               frequency.HighPart, frequency.LowPart);
        // known values: 10,000,000 and 3,000,000 10MHz, 3MHz
        assert(frequency.LowPart % (1000 * 1000) == 0);
        // if we start getting weird frequencies not
        // multiples of MHz num.gcd() approach may need
        // to be revised in favor of num.muldiv64x64()
        freq = frequency.LowPart;
        assert(freq != 0 && freq < (uint32_t)clock.nsec_in_sec);
        // to avoid num.muldiv128:
        uint32_t divider = num.gcd32(clock.nsec_in_sec, freq);
        freq /= divider;
        mul  /= divider;
    }
    uint64_t ns_mul_freq = (uint64_t)qpc.QuadPart * mul;
    return freq == 1 ? ns_mul_freq : ns_mul_freq / freq;
}

// Difference between 1601 and 1970 in microseconds:

const uint64_t clock_epoch_diff_usec = 11644473600000000ULL;

static uint64_t clock_unix_microseconds(void) {
    return clock.microseconds() - clock_epoch_diff_usec;
}

static uint64_t clock_unix_seconds(void) {
    return clock.unix_microseconds() / clock.usec_in_sec;
}

static void clock_test(void) {
    #ifdef UT_TESTS
    // TODO: implement more tests
    uint64_t t0 = clock.nanoseconds();
    uint64_t t1 = clock.nanoseconds();
    int32_t count = 0;
    while (t0 == t1 && count < 1024) {
        t1 = clock.nanoseconds();
        count++;
    }
    swear(t0 != t1, "count: %d t0: %lld t1: %lld", count, t0, t1);
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #endif
}

clock_if clock = {
    .nsec_in_usec      = clock_nsec_in_usec,
    .nsec_in_msec      = clock_nsec_in_msec,
    .nsec_in_sec       = clock_nsec_in_sec,
    .usec_in_msec      = clock_usec_in_msec,
    .msec_in_sec       = clock_msec_in_sec,
    .usec_in_sec       = clock_usec_in_sec,
    .seconds           = clock_seconds,
    .nanoseconds       = clock_nanoseconds,
    .unix_microseconds = clock_unix_microseconds,
    .unix_seconds      = clock_unix_seconds,
    .microseconds      = clock_microseconds_since_epoch,
    .localtime         = clock_localtime,
    .utc               = clock_utc,
    .local             = clock_local,
    .test              = clock_test
};
