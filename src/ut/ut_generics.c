#include "ut/ut.h"

#ifdef UT_TESTS

static void rt_generics_test(void) {
    {
        int8_t a = 10, b = 20;
        rt_swear(rt_max(a++, b++) == 20);
        rt_swear(rt_min(a++, b++) == 11);
    }
    {
        int32_t a = 10, b = 20;
        rt_swear(rt_max(a++, b++) == 20);
        rt_swear(rt_min(a++, b++) == 11);
    }
    {
        fp32_t a = 1.1f, b = 2.2f;
        rt_swear(rt_max(a, b) == b);
        rt_swear(rt_min(a, b) == a);
    }
    {
        fp64_t a = 1.1, b = 2.2;
        rt_swear(rt_max(a, b) == b);
        rt_swear(rt_min(a, b) == a);
    }
    {
        fp32_t a = 1.1f, b = 2.2f;
        rt_swear(rt_max(a, b) == b);
        rt_swear(rt_min(a, b) == a);
    }
    {
        fp64_t a = 1.1, b = 2.2;
        rt_swear(rt_max(a, b) == b);
        rt_swear(rt_min(a, b) == a);
    }
    {
        char a = 1, b = 2;
        rt_swear(rt_max(a, b) == b);
        rt_swear(rt_min(a, b) == a);
    }
    {
        unsigned char a = 1, b = 2;
        rt_swear(rt_max(a, b) == b);
        rt_swear(rt_min(a, b) == a);
    }
    // MS cl.exe version 19.39.33523 has issues with "long":
    // does not pick up int32_t/uint32_t types for "long" and "unsigned long"
    {
        long int a = 1, b = 2;
        rt_swear(rt_max(a, b) == b);
        rt_swear(rt_min(a, b) == a);
    }
    {
        unsigned long a = 1, b = 2;
        rt_swear(rt_max(a, b) == b);
        rt_swear(rt_min(a, b) == a);
    }
    {
        long long a = 1, b = 2;
        rt_swear(rt_max(a, b) == b);
        rt_swear(rt_min(a, b) == a);
    }
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#else

static void rt_generics_test(void) { }

#endif

rt_generics_if rt_generics = {
    .test = rt_generics_test
};
