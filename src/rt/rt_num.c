#include "rt/rt.h"
#include <intrin.h>
//#include <immintrin.h> // _tzcnt_u32

static inline rt_num128_t rt_num_add128_inline(const rt_num128_t a, const rt_num128_t b) {
    rt_num128_t r = a;
    r.hi += b.hi;
    r.lo += b.lo;
    if (r.lo < b.lo) { r.hi++; } // carry
    return r;
}

static inline rt_num128_t rt_num_sub128_inline(const rt_num128_t a, const rt_num128_t b) {
    rt_num128_t r = a;
    r.hi -= b.hi;
    if (r.lo < b.lo) { r.hi--; } // borrow
    r.lo -= b.lo;
    return r;
}

static rt_num128_t rt_num_add128(const rt_num128_t a, const rt_num128_t b) {
    return rt_num_add128_inline(a, b);
}

static rt_num128_t rt_num_sub128(const rt_num128_t a, const rt_num128_t b) {
    return rt_num_sub128_inline(a, b);
}

static rt_num128_t rt_num_mul64x64(uint64_t a, uint64_t b) {
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
    return (rt_num128_t){.lo = low, .hi = high };
}

static inline void rt_num_shift128_left_inline(rt_num128_t* n) {
    const uint64_t top = (1ULL << 63);
    n->hi = (n->hi << 1) | ((n->lo & top) ? 1 : 0);
    n->lo = (n->lo << 1);
}

static inline void rt_num_shift128_right_inline(rt_num128_t* n) {
    const uint64_t top = (1ULL << 63);
    n->lo = (n->lo >> 1) | ((n->hi & 0x1) ? top : 0);
    n->hi = (n->hi >> 1);
}

static inline bool rt_num_less128_inline(const rt_num128_t a, const rt_num128_t b) {
    return a.hi < b.hi || (a.hi == b.hi && a.lo < b.lo);
}

static inline bool rt_num_uint128_high_bit(const rt_num128_t a) {
    return (int64_t)a.hi < 0;
}

static uint64_t rt_num_muldiv128(uint64_t a, uint64_t b, uint64_t divisor) {
    rt_swear(divisor > 0, "divisor: %lld", divisor);
    rt_num128_t r = rt_num.mul64x64(a, b); // reminder: a * b
    uint64_t q = 0; // quotient
    if (r.hi >= divisor) {
        q = UINT64_MAX; // overflow
    } else {
        int32_t  shift = 0;
        rt_num128_t d = { .hi = 0, .lo = divisor };
        while (!rt_num_uint128_high_bit(d) && rt_num_less128_inline(d, r)) {
            rt_num_shift128_left_inline(&d);
            shift++;
        }
        rt_assert(shift <= 64);
        while (shift >= 0 && (d.hi != 0 || d.lo != 0)) {
            if (!rt_num_less128_inline(r, d)) {
                r = rt_num_sub128_inline(r, d);
                rt_assert(shift < 64);
                q |= (1ULL << shift);
            }
            rt_num_shift128_right_inline(&d);
            shift--;
        }
    }
    return q;
}

static uint32_t rt_num_gcd32(uint32_t u, uint32_t v) {
    #pragma push_macro("rt_trailing_zeros")
    #ifdef _M_ARM64
    #define rt_trailing_zeros(x) (_CountTrailingZeros(x))
    #else
    #define rt_trailing_zeros(x) ((int32_t)_tzcnt_u32(x))
    #endif
    if (u == 0) {
        return v;
    } else if (v == 0) {
        return u;
    }
    uint32_t i = rt_trailing_zeros(u);  u >>= i;
    uint32_t j = rt_trailing_zeros(v);  v >>= j;
    uint32_t k = rt_min(i, j);
    for (;;) {
        rt_assert(u % 2 == 1, "u = %d should be odd", u);
        rt_assert(v % 2 == 1, "v = %d should be odd", v);
        if (u > v) { uint32_t swap = u; u = v; v = swap; }
        v -= u;
        if (v == 0) { return u << k; }
        v >>= rt_trailing_zeros(v);
    }
    #pragma pop_macro("rt_trailing_zeros")
}

static uint32_t rt_num_random32(uint32_t* state) {
    // https://gist.github.com/tommyettinger/46a874533244883189143505d203312c
    static rt_thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
    uint32_t z = (*state += 0x6D2B79F5UL);
    z = (z ^ (z >> 15)) * (z | 1UL);
    z ^= z + (z ^ (z >> 7)) * (z | 61UL);
    return z ^ (z >> 14);
}

static uint64_t rt_num_random64(uint64_t *state) {
    // https://gist.github.com/tommyettinger/e6d3e8816da79b45bfe582384c2fe14a
    static rt_thread_local bool started; // first seed must be odd
    if (!started) { started = true; *state |= 1; }
	const uint64_t s = *state;
	const uint64_t z = (s ^ s >> 25) * (*state += 0x6A5D39EAE12657AAULL);
	return z ^ (z >> 22);
}

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function

static uint32_t rt_num_hash32(const char *data, int64_t len) {
    uint32_t hash  = 0x811c9dc5;  // FNV_offset_basis for 32-bit
    uint32_t prime = 0x01000193; // FNV_prime for 32-bit
    if (len > 0) {
        for (int64_t i = 1; i < len; i++) {
            hash ^= (uint32_t)data[i];
            hash *= prime;
        }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) {
            hash ^= (uint32_t)data[i];
            hash *= prime;
        }
    }
    return hash;
}

static uint64_t rt_num_hash64(const char *data, int64_t len) {
    uint64_t hash  = 0xcbf29ce484222325; // FNV_offset_basis for 64-bit
    uint64_t prime = 0x100000001b3;      // FNV_prime for 64-bit
    if (len > 0) {
        for (int64_t i = 0; i < len; i++) {
            hash ^= (uint64_t)data[i];
            hash *= prime;
        }
    } else {
        for (int64_t i = 0; data[i] != 0; i++) {
            hash ^= (uint64_t)data[i];
            hash *= prime;
        }
    }
    return hash;
}

static uint32_t ctz_2(uint32_t x) {
    if (x == 0) return 32;
    unsigned n = 0;
    while ((x & 1) == 0) {
        x >>= 1;
        n++;
    }
    return n;
}

static void rt_num_test(void) {
    #ifdef RT_TESTS
    {
        rt_swear(rt_num.gcd32(1000000000, 24000000) == 8000000);
        // https://asecuritysite.com/encryption/nprimes?y=64
        // https://www.rapidtables.com/convert/number/decimal-to-hex.html
        uint64_t p = 15843490434539008357u; // prime
        uint64_t q = 16304766625841520833u; // prime
        // pq: 258324414073910997987910483408576601381
        //     0xC25778F20853A9A1EC0C27C467C45D25
        rt_num128_t pq = {.hi = 0xC25778F20853A9A1uLL,
                       .lo = 0xEC0C27C467C45D25uLL };
        rt_num128_t p_q = rt_num.mul64x64(p, q);
        rt_swear(p_q.hi == pq.hi && pq.lo == pq.lo);
        uint64_t p1 = rt_num.muldiv128(p, q, q);
        uint64_t q1 = rt_num.muldiv128(p, q, p);
        rt_swear(p1 == p);
        rt_swear(q1 == q);
    }
    #ifdef DEBUG
    enum { n = 100 };
    #else
    enum { n = 10000 };
    #endif
    uint64_t seed64 = 1;
    for (int32_t i = 0; i < n; i++) {
        uint64_t p = rt_num.random64(&seed64);
        uint64_t q = rt_num.random64(&seed64);
        uint64_t p1 = rt_num.muldiv128(p, q, q);
        uint64_t q1 = rt_num.muldiv128(p, q, p);
        rt_swear(p == p1, "0%16llx (0%16llu) != 0%16llx (0%16llu)", p, p1);
        rt_swear(q == q1, "0%16llx (0%16llu) != 0%16llx (0%16llu)", p, p1);
    }
    uint32_t seed32 = 1;
    for (int32_t i = 0; i < n; i++) {
        uint64_t p = rt_num.random32(&seed32);
        uint64_t q = rt_num.random32(&seed32);
        uint64_t r = rt_num.muldiv128(p, q, 1);
        rt_swear(r == p * q);
        // division by the maximum uint64_t value:
        r = rt_num.muldiv128(p, q, UINT64_MAX);
        rt_swear(r == 0);
    }
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
    #endif
}

rt_num_if rt_num = {
    .add128    = rt_num_add128,
    .sub128    = rt_num_sub128,
    .mul64x64  = rt_num_mul64x64,
    .muldiv128 = rt_num_muldiv128,
    .gcd32     = rt_num_gcd32,
    .random32  = rt_num_random32,
    .random64  = rt_num_random64,
    .hash32    = rt_num_hash32,
    .hash64    = rt_num_hash64,
    .test      = rt_num_test
};
