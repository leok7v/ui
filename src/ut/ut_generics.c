#include "ut/ut.h"

#ifdef UT_TESTS

static void ut_generics_test(void) {
    {
        int8_t a = 10, b = 20;
        ut_swear(ut_max(a++, b++) == 20);
        ut_swear(ut_min(a++, b++) == 11);
    }
    {
        int32_t a = 10, b = 20;
        ut_swear(ut_max(a++, b++) == 20);
        ut_swear(ut_min(a++, b++) == 11);
    }
    {
        fp32_t a = 1.1f, b = 2.2f;
        ut_swear(ut_max(a, b) == b);
        ut_swear(ut_min(a, b) == a);
    }
    {
        fp64_t a = 1.1, b = 2.2;
        ut_swear(ut_max(a, b) == b);
        ut_swear(ut_min(a, b) == a);
    }
    {
        fp32_t a = 1.1f, b = 2.2f;
        ut_swear(ut_max(a, b) == b);
        ut_swear(ut_min(a, b) == a);
    }
    {
        fp64_t a = 1.1, b = 2.2;
        ut_swear(ut_max(a, b) == b);
        ut_swear(ut_min(a, b) == a);
    }
    {
        char a = 1, b = 2;
        ut_swear(ut_max(a, b) == b);
        ut_swear(ut_min(a, b) == a);
    }
    {
        unsigned char a = 1, b = 2;
        ut_swear(ut_max(a, b) == b);
        ut_swear(ut_min(a, b) == a);
    }
    // MS cl.exe version 19.39.33523 has issues with "long":
    // does not pick up int32_t/uint32_t types for "long" and "unsigned long"
    {
        long int a = 1, b = 2;
        ut_swear(ut_max(a, b) == b);
        ut_swear(ut_min(a, b) == a);
    }
    {
        unsigned long a = 1, b = 2;
        ut_swear(ut_max(a, b) == b);
        ut_swear(ut_min(a, b) == a);
    }
    {
        long long a = 1, b = 2;
        ut_swear(ut_max(a, b) == b);
        ut_swear(ut_min(a, b) == a);
    }
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_println("done"); }
}

#else

static void ut_generics_test(void) { }

#endif

ut_generics_if ut_generics = {
    .test = ut_generics_test
};
