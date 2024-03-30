#include "rt.h"
#include <immintrin.h> // _tzcnt_u32


// TODO: correct implementation is here
// https://opensource.apple.com/source/Libc/Libc-1044.1.2/gen/nanosleep.c.auto.html

static num128_t num_mul128(uint64_t a, uint64_t b) {
    uint64_t a_lo = (uint32_t)a;
    uint64_t a_hi = a >> 32;
    uint64_t b_lo = (uint32_t)b;
    uint64_t b_hi = b >> 32;
    uint64_t low = a_lo * b_lo;
    uint64_t cross1 = a_hi * b_lo;
    uint64_t cross2 = a_lo * b_hi;
    uint64_t high = a_hi * b_hi;
    // Add cross terms to 'low' and 'high'
    uint64_t cross = cross1 + cross2;
    high += cross >> 32;
    low += (cross << 32);
    // Check for overflow in 'low' and carry to 'high'
    if (low < (cross << 32)) {
        high++;
    }
    num128_t result = {.lo = low, .hi = high, .overflow = (high >> 32) != 0};
    return result;
}

static uint64_t num_muldiv128(uint64_t a, uint64_t b, uint64_t d, bool* overflow) {
    num128_t product = num.mul128(a, b);
    *overflow = product.overflow || product.hi >= d;
    uint64_t quotient = 0;
    if (!*overflow) {
        // Divide the 128-bit product by the divisor using 64-bit division
        quotient = product.hi / d;
        uint64_t remainder = product.hi % d;
        remainder = (remainder << 32) | (product.lo >> 32);
        quotient = (quotient << 32) | (remainder / d);
        remainder = (remainder % d) << 32 | (uint32_t)product.lo;
        quotient = (quotient << 32) | (remainder / d);
    }
    return *overflow ? UINT64_MAX : quotient;
}

#define ctz(x) _tzcnt_u32(x)

static uint32_t num_gcd32(uint32_t u, uint32_t v) {
    uint32_t t = u | v;
    if (u == 0 || v == 0) { return t; }
    int g = ctz(t);
    while (u != 0) {
        u >>= ctz(u);
        v >>= ctz(v);
        if (u >= v) {
            u = (u - v) / 2;
        } else {
            v = (v - u) / 2;
        }
    }
    return v << g;
}

static uint32_t num_random32(uint32_t* state) {
    // https://gist.github.com/tommyettinger/46a874533244883189143505d203312c
    static thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
    uint32_t z = (*state += 0x6D2B79F5UL);
    z = (z ^ (z >> 15)) * (z | 1UL);
    z ^= z + (z ^ (z >> 7)) * (z | 61UL);
    return z ^ (z >> 14);
}

static uint64_t num_random64(uint64_t *state) {
    // https://gist.github.com/tommyettinger/e6d3e8816da79b45bfe582384c2fe14a
    static thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
	const uint64_t s = *state;
	const uint64_t z = (s ^ s >> 25) * (*state += 0x6A5D39EAE12657AAULL);
	return z ^ (z >> 22);
}

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function

static uint32_t num_hash32(const char *data, int64_t len) {
    uint32_t hash  = 0x811c9dc5;  // FNV_offset_basis for 32-bit
    uint32_t prime = 0x01000193; // FNV_prime for 32-bit
    if (len > 0) {
        for (int64_t i = 1; i < len; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    }
    return hash;
}

static uint64_t num_hash64(const char *data, int64_t len) {
    uint64_t hash  = 0xcbf29ce484222325; // FNV_offset_basis for 64-bit
    uint64_t prime = 0x100000001b3;      // FNV_prime for 64-bit
    if (len > 0) {
        for (int64_t i = 0; i < len; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) {
            hash ^= data[i];
            hash *= prime;
        }
    }
    return hash;
}

static void num_test(int32_t verbosity) {
    bool overflow = false;
    // Test multiplication without overflow
    num128_t result = num.mul128(0xFFFFFFFF, 0xFFFFFFFF);
    swear(result.lo == 0xFFFFFFFE00000001,
          "result: 0x%016llX 0x%016llX", result.hi, result.lo);
    swear(result.hi == 0);
    swear(!result.overflow);
    // Test multiplication with overflow
    result = num.mul128(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
    swear(result.overflow);
    // Test division without overflow
    uint64_t quotient = num.muldiv128(0xFFFFFFFF, 0xFFFFFFFF, 2, &overflow);
    swear(quotient == 0x7FFFFFFF80000000,
        "quotient: 0x%016llX", quotient);
    swear(!overflow);
    // Test division with overflow
    quotient = num.muldiv128(0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 2, &overflow);
    swear(overflow);
    // Test with small numbers
    result = num.mul128(123, 456);
    swear(result.lo == 123 * 456);
    swear(result.hi == 0);
    swear(!result.overflow);
    quotient = num.muldiv128(123, 456, 789, &overflow);
    swear(quotient == (123 * 456) / 789);
    swear(!overflow);
    // Test division by 1 (should return the product)
    quotient = num.muldiv128(123456789, 987654321, 1, &overflow);
    swear(quotient == 123456789LL * 987654321LL);
    swear(!overflow);
    // Test division by the maximum uint64_t value (should be 0 if no overflow)
    quotient = num.muldiv128(123456789, 987654321, UINT64_MAX, &overflow);
    swear(quotient == 0);
    swear(!overflow);
    // Test multiplication and division with zero
    result = num.mul128(0, 987654321);
    swear(result.lo == 0);
    swear(result.hi == 0);
    swear(!result.overflow);
    quotient = num.muldiv128(0, 987654321, 123456789, &overflow);
    swear(quotient == 0);
    swear(!overflow);
    // Test division with a divisor larger than the product
    quotient = num.muldiv128(123456789, 987654321,
                123456789987654321ULL, &overflow);
    swear(quotient == 0);
    swear(!overflow);
    if (verbosity > 0) { traceln("done"); }
}

num_if num = {
    .mul128    = num_mul128,
    .muldiv128 = num_muldiv128,
    .gcd32     = num_gcd32,
    .random32  = num_random32,
    .random64  = num_random64,
    .hash32    = num_hash32,
    .hash64    = num_hash64,
    .test      = num_test
};
