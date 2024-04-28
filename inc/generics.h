#pragma once
#include "std.h"

begin_c

// Type aliases for floating-point types similar to <stdint.h>
typedef float  fp32_t;
typedef double fp64_t;

// Most of ut/ui code is written the way of min(a,b) max(a,b)
// not having side effects on the arguments and thus evaluating
// them twice ain't a big deal. However, out of curiosity of
// usefulness of Generic() in C11 standard here is type-safe
// single evaluation of the arguments version of what could
// have been simple minimum and maximum macro definitions.
// Type safety comes with the cost of complexity in puritan
// or stoic language like C:

static inline int8_t   maximum_int8(int8_t x, int8_t y)       { return x > y ? x : y; }
static inline int16_t  maximum_int16(int16_t x, int16_t y)    { return x > y ? x : y; }
static inline int32_t  maximum_int32(int32_t x, int32_t y)    { return x > y ? x : y; }
static inline int64_t  maximum_int64(int64_t x, int64_t y)    { return x > y ? x : y; }
static inline uint8_t  maximum_uint8(uint8_t x, uint8_t y)    { return x > y ? x : y; }
static inline uint16_t maximum_uint16(uint16_t x, uint16_t y) { return x > y ? x : y; }
static inline uint32_t maximum_uint32(uint32_t x, uint32_t y) { return x > y ? x : y; }
static inline uint64_t maximum_uint64(uint64_t x, uint64_t y) { return x > y ? x : y; }
static inline fp32_t   maximum_fp32(fp32_t x, fp32_t y)       { return x > y ? x : y; }
static inline fp64_t   maximum_fp64(fp64_t x, fp64_t y)       { return x > y ? x : y; }
// also need to handle long because cl.exe compiler is slightly confused about it:
static inline long          maximum_long(long x, long y)                    { return x > y ? x : y; }
static inline unsigned long maximum_ulong(unsigned long x, unsigned long y) { return x > y ? x : y; }

static inline int8_t   minimum_int8(int8_t x, int8_t y)       { return x < y ? x : y; }
static inline int16_t  minimum_int16(int16_t x, int16_t y)    { return x < y ? x : y; }
static inline int32_t  minimum_int32(int32_t x, int32_t y)    { return x < y ? x : y; }
static inline int64_t  minimum_int64(int64_t x, int64_t y)    { return x < y ? x : y; }
static inline uint8_t  minimum_uint8(uint8_t x, uint8_t y)    { return x < y ? x : y; }
static inline uint16_t minimum_uint16(uint16_t x, uint16_t y) { return x < y ? x : y; }
static inline uint32_t minimum_uint32(uint32_t x, uint32_t y) { return x < y ? x : y; }
static inline uint64_t minimum_uint64(uint64_t x, uint64_t y) { return x < y ? x : y; }
static inline fp32_t   minimum_fp32(fp32_t x, fp32_t y)       { return x < y ? x : y; }
static inline fp64_t   minimum_fp64(fp64_t x, fp64_t y)       { return x < y ? x : y; }

static inline long          minimum_long(long x, long y)                    { return x < y ? x : y; }
static inline unsigned long minimum_ulong(unsigned long x, unsigned long y) { return x < y ? x : y; }


static inline void     minimum_undefined(void) { }
static inline void     maximum_undefined(void) { }

#define maximum(X, Y) _Generic((X) + (Y), \
    int8_t:   maximum_int8,   \
    int16_t:  maximum_int16,  \
    int32_t:  maximum_int32,  \
    int64_t:  maximum_int64,  \
    uint8_t:  maximum_uint8,  \
    uint16_t: maximum_uint16, \
    uint32_t: maximum_uint32, \
    uint64_t: maximum_uint64, \
    fp32_t:   maximum_fp32,   \
    fp64_t:   maximum_fp64,   \
    long:     maximum_long,   \
    unsigned long: maximum_ulong, \
    default:  maximum_undefined)(X, Y)

#define minimum(X, Y) _Generic((X) + (Y), \
    int8_t:   minimum_int8,   \
    int16_t:  minimum_int16,  \
    int32_t:  minimum_int32,  \
    int64_t:  minimum_int64,  \
    uint8_t:  minimum_uint8,  \
    uint16_t: minimum_uint16, \
    uint32_t: minimum_uint32, \
    uint64_t: minimum_uint64, \
    fp32_t:   minimum_fp32,   \
    fp64_t:   minimum_fp64,   \
    long:     minimum_long,   \
    unsigned long: minimum_ulong, \
    default:  minimum_undefined)(X, Y)

typedef struct generics_if {
    void (*test)(void);
} generics_if;

end_c