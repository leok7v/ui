#pragma once
#include "util/manifest.h"

begin_c

typedef struct {
    uint64_t lo;
    uint64_t hi;
} num128_t; // uint128_t may be supported by compiler

typedef struct {
    num128_t (*add128)(const num128_t a, const num128_t b);
    num128_t (*sub128)(const num128_t a, const num128_t b);
    num128_t (*mul64x64)(uint64_t a, uint64_t b);
    uint64_t (*muldiv128)(uint64_t a, uint64_t b, uint64_t d);
    uint32_t (*gcd32)(uint32_t u, uint32_t v); // greatest common denominator
    // non-crypto strong pseudo-random number generators (thread safe)
    uint32_t (*random32)(uint32_t *state); // "Mulberry32"
    uint64_t (*random64)(uint64_t *state); // "Trust"
    // "FNV-1a" hash functions (if bytes == 0 expects zero terminated string)
    uint32_t (*hash32)(const char* s, int64_t bytes);
    uint64_t (*hash64)(const char* s, int64_t bytes);
    void     (*test)(void);
} num_if;

extern num_if num;

end_c

