#pragma once
#include "rt/rt_std.h"

rt_begin_c

// Most of ut/ui code is written the way of min(a,b) max(a,b)
// not having side effects on the arguments and thus evaluating
// them twice ain't a big deal. However, out of curiosity of
// usefulness of Generic() in C11 standard here is type-safe
// single evaluation of the arguments version of what could
// have been simple minimum and maximum macro definitions.
// Type safety comes with the cost of complexity in puritan
// or stoic language like C:

static inline int8_t   rt_max_int8(int8_t x, int8_t y)       { return x > y ? x : y; }
static inline int16_t  rt_max_int16(int16_t x, int16_t y)    { return x > y ? x : y; }
static inline int32_t  rt_max_int32(int32_t x, int32_t y)    { return x > y ? x : y; }
static inline int64_t  rt_max_int64(int64_t x, int64_t y)    { return x > y ? x : y; }
static inline uint8_t  rt_max_uint8(uint8_t x, uint8_t y)    { return x > y ? x : y; }
static inline uint16_t rt_max_uint16(uint16_t x, uint16_t y) { return x > y ? x : y; }
static inline uint32_t rt_max_uint32(uint32_t x, uint32_t y) { return x > y ? x : y; }
static inline uint64_t rt_max_uint64(uint64_t x, uint64_t y) { return x > y ? x : y; }
static inline fp32_t   rt_max_fp32(fp32_t x, fp32_t y)       { return x > y ? x : y; }
static inline fp64_t   rt_max_fp64(fp64_t x, fp64_t y)       { return x > y ? x : y; }

// MS cl.exe version 19.39.33523 has issues with "long":
// does not pick up int32_t/uint32_t types for "long" and "unsigned long"
// need to handle long / unsigned long separately:

static inline long          rt_max_long(long x, long y)                    { return x > y ? x : y; }
static inline unsigned long rt_max_ulong(unsigned long x, unsigned long y) { return x > y ? x : y; }

static inline int8_t   rt_min_int8(int8_t x, int8_t y)       { return x < y ? x : y; }
static inline int16_t  rt_min_int16(int16_t x, int16_t y)    { return x < y ? x : y; }
static inline int32_t  rt_min_int32(int32_t x, int32_t y)    { return x < y ? x : y; }
static inline int64_t  rt_min_int64(int64_t x, int64_t y)    { return x < y ? x : y; }
static inline uint8_t  rt_min_uint8(uint8_t x, uint8_t y)    { return x < y ? x : y; }
static inline uint16_t rt_min_uint16(uint16_t x, uint16_t y) { return x < y ? x : y; }
static inline uint32_t rt_min_uint32(uint32_t x, uint32_t y) { return x < y ? x : y; }
static inline uint64_t rt_min_uint64(uint64_t x, uint64_t y) { return x < y ? x : y; }
static inline fp32_t   rt_min_fp32(fp32_t x, fp32_t y)       { return x < y ? x : y; }
static inline fp64_t   rt_min_fp64(fp64_t x, fp64_t y)       { return x < y ? x : y; }

static inline long          rt_min_long(long x, long y)                    { return x < y ? x : y; }
static inline unsigned long rt_min_ulong(unsigned long x, unsigned long y) { return x < y ? x : y; }


static inline void rt_min_undefined(void) { }
static inline void rt_max_undefined(void) { }

#define rt_max(X, Y) _Generic((X) + (Y), \
    int8_t:   rt_max_int8,   \
    int16_t:  rt_max_int16,  \
    int32_t:  rt_max_int32,  \
    int64_t:  rt_max_int64,  \
    uint8_t:  rt_max_uint8,  \
    uint16_t: rt_max_uint16, \
    uint32_t: rt_max_uint32, \
    uint64_t: rt_max_uint64, \
    fp32_t:   rt_max_fp32,   \
    fp64_t:   rt_max_fp64,   \
    long:     rt_max_long,   \
    unsigned long: rt_max_ulong, \
    default:  rt_max_undefined)(X, Y)

#define rt_min(X, Y) _Generic((X) + (Y), \
    int8_t:   rt_min_int8,   \
    int16_t:  rt_min_int16,  \
    int32_t:  rt_min_int32,  \
    int64_t:  rt_min_int64,  \
    uint8_t:  rt_min_uint8,  \
    uint16_t: rt_min_uint16, \
    uint32_t: rt_min_uint32, \
    uint64_t: rt_min_uint64, \
    fp32_t:   rt_min_fp32,   \
    fp64_t:   rt_min_fp64,   \
    long:     rt_min_long,   \
    unsigned long: rt_min_ulong, \
    default:  rt_min_undefined)(X, Y)

// The expression (X) + (Y) is used in _Generic primarily for type promotion
// and compatibility between different types of the two operands. In C, when
// you perform an arithmetic operation like addition between two variables,
// the types of these variables undergo implicit conversions to a common type
// according to the usual arithmetic conversions defined in the C standard.
// This helps ensure that:
//
// Type Compatibility: The macro works correctly even if X and Y are of
// different types. By using (X) + (Y), both X and Y are promoted to a common
// type, which ensures that the macro selects the appropriate function
// that can handle this common type.
//
// Type Safety: It ensures that the selected function can handle the type
// resulting from the operation, thereby preventing type mismatches that
// could lead to undefined behavior or compilation errors.

typedef struct {
    void (*test)(void);
} rt_generics_if;

extern rt_generics_if rt_generics;

rt_end_c
