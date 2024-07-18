#include "ut/ut.h"
#include "ut/ut_win32.h"

enum {
    ut_clock_nsec_in_usec = 1000, // nano in micro
    ut_clock_nsec_in_msec = ut_clock_nsec_in_usec * 1000, // nano in milli
    ut_clock_nsec_in_sec  = ut_clock_nsec_in_msec * 1000,
    ut_clock_usec_in_msec = 1000, // micro in mill
    ut_clock_msec_in_sec  = 1000, // milli in sec
    ut_clock_usec_in_sec  = ut_clock_usec_in_msec * ut_clock_msec_in_sec // micro in sec
};

static uint64_t ut_clock_microseconds_since_epoch(void) { // NOT monotonic
    FILETIME ft; // time in 100ns interval (tenth of microsecond)
    // since 12:00 A.M. January 1, 1601 Coordinated Universal Time (UTC)
    GetSystemTimePreciseAsFileTime(&ft);
    uint64_t microseconds =
        (((uint64_t)ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10;
    assert(microseconds > 0);
    return microseconds;
}

static uint64_t ut_clock_localtime(void) {
    TIME_ZONE_INFORMATION tzi; // UTC = local time + bias
    GetTimeZoneInformation(&tzi);
    uint64_t bias = (uint64_t)tzi.Bias * 60LL * 1000 * 1000; // in microseconds
    return ut_clock_microseconds_since_epoch() - bias;
}

static void ut_clock_utc(uint64_t microseconds,
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

static void ut_clock_local(uint64_t microseconds,
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

static fp64_t ut_clock_seconds(void) { // since_boot
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static fp64_t one_over_freq;
    if (one_over_freq == 0) {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        one_over_freq = 1.0 / (fp64_t)frequency.QuadPart;
    }
    return (fp64_t)qpc.QuadPart * one_over_freq;
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
// for ut_clock.nanoseconds() to overflow
//
// for divider = ut_num.gcd32(nsec_in_sec, freq) below and 10MHz timer
// the actual duration is shorter because of (mul == 100)
//    (uint64_t)qpc.QuadPart * mul
// 64 bit overflow and is about 5.8 years.
//
// In a long running code like services is advisable to use
// ut_clock.nanoseconds() to measure only deltas and pay close attention
// to the wrap around despite of 5 years monotony

static uint64_t ut_clock_nanoseconds(void) {
    LARGE_INTEGER qpc;
    QueryPerformanceCounter(&qpc);
    static uint32_t freq;
    static uint32_t mul = ut_clock_nsec_in_sec;
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
        // multiples of MHz ut_num.gcd() approach may need
        // to be revised in favor of ut_num.muldiv64x64()
        freq = frequency.LowPart;
        assert(freq != 0 && freq < (uint32_t)ut_clock.nsec_in_sec);
        // to avoid ut_num.muldiv128:
        uint32_t divider = ut_num.gcd32((uint32_t)ut_clock.nsec_in_sec, freq);
        freq /= divider;
        mul  /= divider;
    }
    uint64_t ns_mul_freq = (uint64_t)qpc.QuadPart * mul;
    return freq == 1 ? ns_mul_freq : ns_mul_freq / freq;
}

// Difference between 1601 and 1970 in microseconds:

static const uint64_t ut_clock_epoch_diff_usec = 11644473600000000ULL;

static uint64_t ut_clock_unix_microseconds(void) {
    return ut_clock.microseconds() - ut_clock_epoch_diff_usec;
}

static uint64_t ut_clock_unix_seconds(void) {
    return ut_clock.unix_microseconds() / (uint64_t)ut_clock.usec_in_sec;
}

static void ut_clock_test(void) {
    #ifdef UT_TESTS
    // TODO: implement more tests
    uint64_t t0 = ut_clock.nanoseconds();
    uint64_t t1 = ut_clock.nanoseconds();
    int32_t count = 0;
    while (t0 == t1 && count < 1024) {
        t1 = ut_clock.nanoseconds();
        count++;
    }
    swear(t0 != t1, "count: %d t0: %lld t1: %lld", count, t0, t1);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_println("done"); }
    #endif
}

ut_clock_if ut_clock = {
    .nsec_in_usec      = ut_clock_nsec_in_usec,
    .nsec_in_msec      = ut_clock_nsec_in_msec,
    .nsec_in_sec       = ut_clock_nsec_in_sec,
    .usec_in_msec      = ut_clock_usec_in_msec,
    .msec_in_sec       = ut_clock_msec_in_sec,
    .usec_in_sec       = ut_clock_usec_in_sec,
    .seconds           = ut_clock_seconds,
    .nanoseconds       = ut_clock_nanoseconds,
    .unix_microseconds = ut_clock_unix_microseconds,
    .unix_seconds      = ut_clock_unix_seconds,
    .microseconds      = ut_clock_microseconds_since_epoch,
    .localtime         = ut_clock_localtime,
    .utc               = ut_clock_utc,
    .local             = ut_clock_local,
    .test              = ut_clock_test
};
