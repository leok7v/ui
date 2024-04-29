#include "ut/ut.h"


#ifdef UT_TESTS

static void generics_test(void) {
    {
        int8_t a = 10, b = 20;
        swear(maximum(a++, b++) == 20);
        swear(minimum(a++, b++) == 11);
    }
    {
        int32_t a = 10, b = 20;
        swear(maximum(a++, b++) == 20);
        swear(minimum(a++, b++) == 11);
    }
    {
        fp32_t a = 1.1f, b = 2.2f;
        swear(maximum(a, b) == b);
        swear(minimum(a, b) == a);
    }
    {
        fp64_t a = 1.1, b = 2.2;
        swear(maximum(a, b) == b);
        swear(minimum(a, b) == a);
    }
    {
        fp32_t a = 1.1f, b = 2.2f;
        swear(maximum(a, b) == b);
        swear(minimum(a, b) == a);
    }
    {
        fp64_t a = 1.1, b = 2.2;
        swear(maximum(a, b) == b);
        swear(minimum(a, b) == a);
    }
    {
        char a = 1, b = 2;
        swear(maximum(a, b) == b);
        swear(minimum(a, b) == a);
    }
    {
        unsigned char a = 1, b = 2;
        swear(maximum(a, b) == b);
        swear(minimum(a, b) == a);
    }
    // MS cl.exe version 19.39.33523 has issues with "long":
    // does not pick up int32_t/uint32_t types for "long" and "unsigned long"
    {
        long int a = 1, b = 2;
        swear(maximum(a, b) == b);
        swear(minimum(a, b) == a);
    }
    {
        unsigned long a = 1, b = 2;
        swear(maximum(a, b) == b);
        swear(minimum(a, b) == a);
    }
    {
        long long a = 1, b = 2;
        swear(maximum(a, b) == b);
        swear(minimum(a, b) == a);
    }
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void generics_test(void) { }

#endif

generics_if generics = {
    .test = generics_test
};
