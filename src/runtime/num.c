#include "runtime/runtime.h"
#include <immintrin.h> // _tzcnt_u32

static inline num128_t num_add128_inline(const num128_t a, const num128_t b) {
    num128_t r = a;
    r.hi += b.hi;
    r.lo += b.lo;
    if (r.lo < b.lo) { r.hi++; } // carry
    return r;
}

static inline num128_t num_sub128_inline(const num128_t a, const num128_t b) {
    num128_t r = a;
    r.hi -= b.hi;
    if (r.lo < b.lo) { r.hi--; } // borrow
    r.lo -= b.lo;
    return r;
}

static num128_t num_add128(const num128_t a, const num128_t b) {
    return num_add128_inline(a, b);
}

static num128_t num_sub128(const num128_t a, const num128_t b) {
    return num_sub128_inline(a, b);
}

static num128_t num_mul64x64(uint64_t a, uint64_t b) {
    uint64_t a_lo = (uint32_t)a;
    uint64_t a_hi = a >> 32;
    uint64_t b_lo = (uint32_t)b;
    uint64_t b_hi = b >> 32;
    uint64_t low = a_lo * b_lo;
    uint64_t cross1 = a_hi * b_lo;
    uint64_t cross2 = a_lo * b_hi;
    uint64_t high = a_hi * b_hi;
    // this cannot overflow as (2^32-1)^2 + 2^32-1 < 2^64-1
    cross1 += low >> 32;
    // this one can overflow
    cross1 += cross2;
    // propagate the carry if any
    high += ((uint64_t)(cross1 < cross2 != 0)) << 32;
    high = high + (cross1 >> 32);
    low = ((cross1 & 0xFFFFFFFF) << 32) + (low & 0xFFFFFFFF);
    return (num128_t){.lo = low, .hi = high };
}

static inline void num_shift128_left_inline(num128_t* n) {
    const uint64_t top = (1ULL << 63);
    n->hi = (n->hi << 1) | ((n->lo & top) ? 1 : 0);
    n->lo = (n->lo << 1);
}

static inline void num_shift128_right_inline(num128_t* n) {
    const uint64_t top = (1ULL << 63);
    n->lo = (n->lo >> 1) | ((n->hi & 0x1) ? top : 0);
    n->hi = (n->hi >> 1);
}

static inline bool num_less128_inline(const num128_t a, const num128_t b) {
    return a.hi < b.hi || (a.hi == b.hi && a.lo < b.lo);
}

static inline bool num_uint128_high_bit(const num128_t a) {
    return (int64_t)a.hi < 0;
}

static uint64_t num_muldiv128(uint64_t a, uint64_t b, uint64_t divisor) {
    swear(divisor > 0, "divisor: %lld", divisor);
    num128_t r = num.mul64x64(a, b); // reminder: a * b
    uint64_t q = 0; // quotient
    if (r.hi >= divisor) {
        q = UINT64_MAX; // overflow
    } else {
        int32_t  shift = 0;
        num128_t d = { .hi = 0, .lo = divisor };
        while (!num_uint128_high_bit(d) && num_less128_inline(d, r)) {
            num_shift128_left_inline(&d);
            shift++;
        }
        assert(shift <= 64);
        while (shift >= 0 && (d.hi != 0 || d.lo != 0)) {
            if (!num_less128_inline(r, d)) {
                r = num_sub128_inline(r, d);
                assert(shift < 64);
                q |= (1ULL << shift);
            }
            num_shift128_right_inline(&d);
            shift--;
        }
    }
    return q;
}

#define ctz(x) _tzcnt_u32(x)

static uint32_t num_gcd32(uint32_t u, uint32_t v) {
    uint32_t t = u | v;
    if (u == 0 || v == 0) { return t; }
    int32_t g = ctz(t);
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

static void num_test(void) {
    #ifdef RUNTIME_TESTS
    {
        // https://asecuritysite.com/encryption/nprimes?y=64
        // https://www.rapidtables.com/convert/number/decimal-to-hex.html
        uint64_t p = 15843490434539008357u; // prime
        uint64_t q = 16304766625841520833u; // prime
        // pq: 258324414073910997987910483408576601381
        //     0xC25778F20853A9A1EC0C27C467C45D25
        num128_t pq = {.hi = 0xC25778F20853A9A1uLL,
                       .lo = 0xEC0C27C467C45D25uLL };
        num128_t p_q = num.mul64x64(p, q);
        swear(p_q.hi == pq.hi && pq.lo == pq.lo);
        uint64_t p1 = num.muldiv128(p, q, q);
        uint64_t q1 = num.muldiv128(p, q, p);
        swear(p1 == p);
        swear(q1 == q);
    }
    #ifdef DEBUG
    enum { n = 100 };
    #else
    enum { n = 10000 };
    #endif
    uint64_t seed64 = 1;
    for (int32_t i = 0; i < n; i++) {
        uint64_t p = num.random64(&seed64);
        uint64_t q = num.random64(&seed64);
        uint64_t p1 = num.muldiv128(p, q, q);
        uint64_t q1 = num.muldiv128(p, q, p);
        swear(p == p1, "0%16llx (0%16llu) != 0%16llx (0%16llu)", p, p1);
        swear(q == q1, "0%16llx (0%16llu) != 0%16llx (0%16llu)", p, p1);
    }
    uint32_t seed32 = 1;
    for (int32_t i = 0; i < n; i++) {
        uint64_t p = num.random32(&seed32);
        uint64_t q = num.random32(&seed32);
        uint64_t r = num.muldiv128(p, q, 1);
        swear(r == p * q);
        // division by the maximum uint64_t value:
        r = num.muldiv128(p, q, UINT64_MAX);
        swear(r == 0);
    }
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #endif
}

num_if num = {
    .add128    = num_add128,
    .sub128    = num_sub128,
    .mul64x64  = num_mul64x64,
    .muldiv128 = num_muldiv128,
    .gcd32     = num_gcd32,
    .random32  = num_random32,
    .random64  = num_random64,
    .hash32    = num_hash32,
    .hash64    = num_hash64,
    .test      = num_test
};
