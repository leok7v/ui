#pragma once
#include "manifest.h"

begin_c

typedef struct {
    uint64_t lo;
    uint64_t hi;
    uint64_t overflow; // to avoid padding
} num128_t; // uint128_t may be supported by compiler

typedef struct {
    num128_t (*mul128)(uint64_t a, uint64_t b);
    uint64_t (*muldiv128)(uint64_t a, uint64_t b, uint64_t d, bool* overflow);
    uint32_t (*gcd32)(uint32_t u, uint32_t v); // greatest common denominator
    // non-crypto strong pseudo-random number generators (thread safe)
    uint32_t (*random32)(uint32_t *state); // "Mulberry32"
    uint64_t (*random64)(uint64_t *state); // "Trust"
    // "FNV-1a" hash functions (if bytes == 0 expects zero terminated string)
    uint32_t (*hash32)(const char* s, int64_t bytes);
    uint64_t (*hash64)(const char* s, int64_t bytes);
    void     (*test)(int32_t verbosity);
} num_if;

extern num_if num;

end_c

