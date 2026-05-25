#ifndef core_definition
#define core_definition

// __________________________________ core.h __________________________________

// Fundamental collections: growable arrays, a NUL-terminated byte buffer
// (chars), an owned-string vector (text), an open-addressed hash map, and
// a small PRNG.
//
// Standalone C: depends only on the C standard library and compiles
// cleanly on Windows (MSVC /std:c17), macOS (clang) and Linux (gcc/clang).
// No dependency on the rt_* runtime.

#ifndef CORE_H
#define CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__)
#define CORE_PRINTF_ATTR(fmt, va) __attribute__((format(printf, fmt, va)))
#else
#define CORE_PRINTF_ATTR(fmt, va)
#endif

// Treat allocation failure as fatal (aborts). Returns `p` unchanged when
// non-NULL so it can wrap malloc/realloc inline. Implemented in core.c.
void * core_oom(void * p);

// ============================================================================
// arrays -- generic {data, count, capacity} growth
// ============================================================================

// Generic grow for any {data, count, capacity}-shaped struct. Element size
// comes from sizeof(a->data[0]) -- that walks through the typed pointer so
// we never need a (T) parameter or a (void*) cast, which would re-introduce
// the strict-aliasing pitfall gcc -O2 exploits.
//
// Pre-conditions (asserted in debug, compiled out under -DNDEBUG):
//   - if data == NULL : count == 0 && capacity == 0  (cleanly empty)
//   - if data != NULL : capacity > 0 && count <= capacity
// Catches the easy misuse -- a stack container without an explicit {0}
// initializer, or a malloc without memset -- which would otherwise read
// garbage count/capacity.
//
// Post-condition: data != NULL && capacity >= need (core_oom aborts on
// malloc/realloc failure, so we never return with data == NULL).
#define any_grow(a, need) do { \
    assert((a)->data \
           ? ((a)->capacity > 0 && (a)->count <= (a)->capacity) \
           : ((a)->capacity == 0 && (a)->count == 0)); \
    if (!(a)->data) { \
        (a)->capacity = (need); \
        (a)->data = core_oom(malloc((need) * sizeof((a)->data[0]))); \
    } else if ((need) > (a)->capacity) { \
        (a)->capacity = (need) * 2; \
        (a)->data = core_oom(realloc((a)->data, (a)->capacity * sizeof((a)->data[0]))); \
    } \
    assert((a)->data != NULL && (a)->capacity >= (need)); \
} while (0)

#define define_array(T, name) \
struct name { T * data; size_t count; size_t capacity; }; \
static inline void name##_grow(struct name * a, size_t need) { \
    any_grow(a, need); \
} \
static inline void name##_put(struct name * a, T v) { \
    name##_grow(a, a->count + 1); \
    a->data[a->count++] = v; \
} \
static inline void name##_free(struct name * a) { \
    free(a->data); \
    a->data = NULL; \
    a->count = 0; \
    a->capacity = 0; \
} \
struct name##_swallow_semicolon

// ============================================================================
// chars -- grown-on-demand, always NUL-terminated byte buffer
// ============================================================================

struct chars {
    char * data;
    size_t count;
    size_t capacity;
};

void chars_grow(struct chars * s, size_t need);
void chars_put(struct chars * s, const char * d, size_t count);
void chars_free(struct chars * s);
void chars_puts(struct chars * s, const char * a);
void chars_vprintf(struct chars * s, const char * format, va_list vl);
void chars_printf(struct chars * s, const char * format, ...) CORE_PRINTF_ATTR(2, 3);

// ============================================================================
// text -- append-only vector of owned C strings
// ============================================================================

struct text {
    char ** data;
    size_t  count;
    size_t  capacity;
};

void text_grow(struct text * t, size_t need);
void text_put(struct text * t, const char * s, size_t n);
void text_puts(struct text * t, const char * s);
void text_free(struct text * t);

// ============================================================================
// maps -- open-addressed hash map (int64 or struct chars keys)
// ============================================================================

enum map_key { MAP_KEY_INT, MAP_KEY_CHARS };

#define MAP_EMPTY     0
#define MAP_LIVE      1
#define MAP_TOMBSTONE 2

struct map {
    uint8_t *    states;
    void *       keys;
    void *       values;
    size_t       count;
    size_t       capacity;
    size_t       key_size;
    size_t       value_size;
    enum map_key key_kind;
    void         (*value_free)(void *);
};

// Optional explicit setup. Needed only when storing values that own
// resources (pass `value_free`) or when using the generic map_put/map_get
// with a custom key kind. For the common case a zero-initialized map plus
// the typed accessors below is enough -- see map_puti/map_puts.
void map_init(struct map * m, enum map_key kk, size_t ks, size_t vs,
              void (*vf)(void *));

// Generic key/value access. Requires the map to be initialized (via
// map_init or a prior typed put): asserts value_size > 0.
void * map_put(struct map * m, const void * k, const void * v);
void * map_get(struct map * m, const void * k);
void   map_remove(struct map * m, const void * k);
void   map_free(struct map * m);
void   map_for_each(struct map * m,
                    void (*fn)(const void * k, void * v, void * ctx),
                    void * ctx);

// Typed accessors. These work on a zero-initialized map -- `struct map m =
// {0};` -- with no map_init() call: the first put captures the key kind /
// size and the value size (taken via sizeof at the call site, so `v` must
// be a typed pointer to the value, e.g. &my_int). value_free defaults to
// NULL; if stored values own resources set `m.value_free` before the first
// put (e.g. m.value_free = chars_free_v).
void * map_put_int(struct map * m, int64_t k, const void * v, size_t vs);
void * map_get_int(struct map * m, int64_t k);
void   map_remove_int(struct map * m, int64_t k);
void * map_put_str(struct map * m, const char * k, const void * v, size_t vs);
void * map_get_str(struct map * m, const char * k);
void   map_remove_str(struct map * m, const char * k);

#define map_puti(m, k, v)    map_put_int((m), (int64_t)(k), (v), sizeof(*(v)))
#define map_geti(m, k)       map_get_int((m), (int64_t)(k))
#define map_removei(m, k)    map_remove_int((m), (int64_t)(k))
#define map_puts(m, k, v)    map_put_str((m), (k), (v), sizeof(*(v)))
#define map_gets(m, k)       map_get_str((m), (k))
#define map_removes(m, k)    map_remove_str((m), (k))

// Convenience value_free for maps whose values are `struct chars`.
void chars_free_v(void * s);

// ============================================================================
// rng -- xorshift64*; good enough for token sampling
// ============================================================================

struct rng { uint64_t state; };

void     rng_seed(struct rng * r, uint64_t s);
uint64_t rng_next(struct rng * r);
float    rng_uniform(struct rng * r);

#ifdef __cplusplus
}
#endif
#endif // CORE_H

#endif // core_definition

#if defined(core_implementation) && !defined(core_implementation_included)
#define core_implementation_included
// __________________________________ core.c __________________________________

// Fundamental collections -- implementation. See inc/core.h.
// Standalone C; depends only on the C standard library.


#include <stdio.h>
#include <string.h>

// ============================================================================
// allocation
// ============================================================================

// Treat OOM as fatal; abort() so we don't run atexit handlers on a
// half-built program state.
void * core_oom(void * p) {
    if (!p) { fprintf(stderr, "OOM"); abort(); }
    return p;
}

// ============================================================================
// chars
// ============================================================================

// Uses any_grow with sizeof(s->data[0]) == 1 (char). An earlier shape
// delegated through a (void*) cast which violates strict aliasing -- gcc
// -O2 then silently skips the write-back to s->data, leaving the buffer
// NULL. Keep the typed any_grow.
void chars_grow(struct chars * s, size_t need) {
    any_grow(s, need);
}

void chars_put(struct chars * s, const char * d, size_t count) {
    chars_grow(s, s->count + count + 1);
    if (s->data) {
        memcpy(s->data + s->count, d, count);
        s->count += count;
        s->data[s->count] = '\0';
    }
}

void chars_free(struct chars * s) {
    free(s->data);
    s->data = NULL;
    s->count = 0;
    s->capacity = 0;
}

void chars_puts(struct chars * s, const char * a) {
    chars_put(s, a, strlen(a));
}

void chars_vprintf(struct chars * s, const char * format, va_list vl) {
    va_list cp;
    va_copy(cp, vl);
    int r = vsnprintf(NULL, 0, format, cp);
    va_end(cp);
    if (r > 0) {
        size_t n = (size_t)r;
        chars_grow(s, s->count + n + 1);
        vsnprintf(s->data + s->count, n + 1, format, vl);
        s->count += n;
    }
}

void chars_printf(struct chars * s, const char * format, ...) {
    va_list vl;
    va_start(vl, format);
    chars_vprintf(s, format, vl);
    va_end(vl);
}

// ============================================================================
// text
// ============================================================================

void text_grow(struct text * t, size_t need) {
    any_grow(t, need);
}

void text_put(struct text * t, const char * s, size_t n) {
    text_grow(t, t->count + 1);
    char * copy = core_oom(malloc(n + 1));
    memcpy(copy, s, n);
    copy[n] = '\0';
    t->data[t->count++] = copy;
}

void text_puts(struct text * t, const char * s) {
    text_put(t, s, strlen(s));
}

void text_free(struct text * t) {
    for (size_t i = 0; i < t->count; i++) { free(t->data[i]); }
    free(t->data);
    t->data = NULL;
    t->count = 0;
    t->capacity = 0;
}

// ============================================================================
// maps
// ============================================================================

static inline uint64_t map_hash_i(int64_t k) {
    uint64_t x = (uint64_t)k;
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

static inline uint64_t map_hash_s(const struct chars * k) {
    uint64_t h = 0xcbf29ce484222325ULL; // FNV-1a offset
    for (size_t i = 0; i < k->count; i++) {
        h ^= (uint8_t)k->data[i];
        h *= 0x100000001b3ULL; // FNV prime
    }
    return h;
}

static inline uint64_t map_hash(const struct map * m, const void * k) {
    uint64_t h = 0;
    if (m->key_kind == MAP_KEY_INT) {
        h = map_hash_i(*(const int64_t *)k);
    } else {
        h = map_hash_s((const struct chars *)k);
    }
    return h;
}

static inline bool map_eq(const struct map * m, const void * a,
                          const void * b) {
    bool r = false;
    if (m->key_kind == MAP_KEY_INT) {
        r = *(const int64_t *)a == *(const int64_t *)b;
    } else {
        const struct chars * x = a;
        const struct chars * y = b;
        r = x->count == y->count
            && memcmp(x->data, y->data, x->count) == 0;
    }
    return r;
}

static inline void * map_k(struct map * m, size_t i) {
    return (char *)m->keys + i * m->key_size;
}

static inline void * map_v(struct map * m, size_t i) {
    return (char *)m->values + i * m->value_size;
}

static inline void map_key_copy(struct map * m, void * dst,
                                const void * src) {
    if (m->key_kind == MAP_KEY_INT) {
        memcpy(dst, src, sizeof(int64_t));
    } else {
        const struct chars * s = src;
        struct chars * d = dst;
        *d = (struct chars){0};
        chars_put(d, s->data, s->count);
    }
}

static inline void map_key_free(struct map * m, void * k) {
    if (m->key_kind == MAP_KEY_CHARS) {
        chars_free(k);
    }
}

void map_init(struct map * m, enum map_key kk,
              size_t ks, size_t vs, void (*vf)(void *)) {
    m->states     = NULL;
    m->keys       = NULL;
    m->values     = NULL;
    m->count      = 0;
    m->capacity   = 0;
    m->key_size   = ks;
    m->value_size = vs;
    m->key_kind   = kk;
    m->value_free = vf;
}

// Lazy initialization for the typed accessors: a zero-initialized map has
// value_size == 0, which is the unambiguous "not yet configured" marker
// (a real value type is always > 0 bytes). Sets key kind/size and value
// size on first use; leaves value_free as the caller left it (NULL for a
// {0} map). A no-op once configured, so it composes with map_init.
static void map_lazy_init(struct map * m, enum map_key kk,
                          size_t ks, size_t vs) {
    if (m->value_size == 0) {
        m->key_kind   = kk;
        m->key_size   = ks;
        m->value_size = vs;
    }
}

static void map_grow(struct map * m, size_t new_cap) {
    uint8_t * ns = core_oom(calloc(new_cap, 1));
    void *    nk = core_oom(malloc(new_cap * m->key_size));
    void *    nv = core_oom(malloc(new_cap * m->value_size));
    size_t mask = new_cap - 1;
    for (size_t i = 0; i < m->capacity; i++) {
        if (m->states[i] == MAP_LIVE) {
            void * ok = map_k(m, i);
            size_t j = (size_t)map_hash(m, ok) & mask;
            while (ns[j] != MAP_EMPTY) { j = (j + 1) & mask; }
            ns[j] = MAP_LIVE;
            memcpy((char *)nk + j * m->key_size, ok, m->key_size);
            memcpy((char *)nv + j * m->value_size,
                   map_v(m, i), m->value_size);
        }
    }
    free(m->states);
    free(m->keys);
    free(m->values);
    m->states   = ns;
    m->keys     = nk;
    m->values   = nv;
    m->capacity = new_cap;
}

void * map_put(struct map * m, const void * k, const void * v) {
    assert(m->value_size > 0 &&
           "map_put: call map_init() or a typed accessor (map_puti/map_puts)");
    void * r = NULL;
    if (m->capacity == 0) {
        map_grow(m, 16);
    } else if ((m->count + 1) * 4 > m->capacity * 3) {
        map_grow(m, m->capacity * 2);
    }
    size_t mask = m->capacity - 1;
    size_t j    = (size_t)map_hash(m, k) & mask;
    size_t tomb = (size_t)-1;
    while (m->states[j] != MAP_EMPTY
           && !(m->states[j] == MAP_LIVE && map_eq(m, map_k(m, j), k))) {
        if (m->states[j] == MAP_TOMBSTONE && tomb == (size_t)-1) {
            tomb = j;
        }
        j = (j + 1) & mask;
    }
    if (m->states[j] == MAP_LIVE) {
        void * vs = map_v(m, j);
        if (m->value_free) { m->value_free(vs); }
        memcpy(vs, v, m->value_size);
        r = vs;
    } else {
        if (tomb != (size_t)-1) { j = tomb; }
        m->states[j] = MAP_LIVE;
        map_key_copy(m, map_k(m, j), k);
        memcpy(map_v(m, j), v, m->value_size);
        m->count++;
        r = map_v(m, j);
    }
    return r;
}

void * map_get(struct map * m, const void * k) {
    void * r = NULL;
    if (m->capacity > 0) {
        size_t mask = m->capacity - 1;
        size_t j    = (size_t)map_hash(m, k) & mask;
        while (m->states[j] != MAP_EMPTY
               && !(m->states[j] == MAP_LIVE && map_eq(m, map_k(m, j), k))) {
            j = (j + 1) & mask;
        }
        if (m->states[j] == MAP_LIVE) {
            r = map_v(m, j);
        }
    }
    return r;
}

void map_remove(struct map * m, const void * k) {
    if (m->capacity > 0) {
        size_t mask = m->capacity - 1;
        size_t j    = (size_t)map_hash(m, k) & mask;
        while (m->states[j] != MAP_EMPTY
               && !(m->states[j] == MAP_LIVE && map_eq(m, map_k(m, j), k))) {
            j = (j + 1) & mask;
        }
        if (m->states[j] == MAP_LIVE) {
            if (m->value_free) { m->value_free(map_v(m, j)); }
            map_key_free(m, map_k(m, j));
            m->states[j] = MAP_TOMBSTONE;
            m->count--;
        }
    }
}

void map_free(struct map * m) {
    if (m->capacity > 0) {
        for (size_t i = 0; i < m->capacity; i++) {
            if (m->states[i] == MAP_LIVE) {
                if (m->value_free) { m->value_free(map_v(m, i)); }
                map_key_free(m, map_k(m, i));
            }
        }
        free(m->states);
        free(m->keys);
        free(m->values);
    }
    m->states   = NULL;
    m->keys     = NULL;
    m->values   = NULL;
    m->count    = 0;
    m->capacity = 0;
}

void map_for_each(struct map * m,
                  void (*fn)(const void * k, void * v, void * ctx),
                  void * ctx) {
    for (size_t i = 0; i < m->capacity; i++) {
        if (m->states[i] == MAP_LIVE) {
            fn(map_k(m, i), map_v(m, i), ctx);
        }
    }
}

void * map_put_int(struct map * m, int64_t k, const void * v, size_t vs) {
    map_lazy_init(m, MAP_KEY_INT, sizeof(int64_t), vs);
    return map_put(m, &k, v);
}

void * map_get_int(struct map * m, int64_t k) {
    return map_get(m, &k);
}

void map_remove_int(struct map * m, int64_t k) {
    map_remove(m, &k);
}

void * map_put_str(struct map * m, const char * k, const void * v, size_t vs) {
    map_lazy_init(m, MAP_KEY_CHARS, sizeof(struct chars), vs);
    struct chars tmp = {0};
    tmp.data  = (char *)(uintptr_t)k;
    tmp.count = strlen(k);
    return map_put(m, &tmp, v);
}

void * map_get_str(struct map * m, const char * k) {
    struct chars tmp = {0};
    tmp.data  = (char *)(uintptr_t)k;
    tmp.count = strlen(k);
    return map_get(m, &tmp);
}

void map_remove_str(struct map * m, const char * k) {
    struct chars tmp = {0};
    tmp.data  = (char *)(uintptr_t)k;
    tmp.count = strlen(k);
    map_remove(m, &tmp);
}

void chars_free_v(void * s) {
    chars_free((struct chars *)s);
}

// ============================================================================
// rng
// ============================================================================

void rng_seed(struct rng * r, uint64_t s) {
    r->state = s ? s : 0xdeadbeefcafef00dULL;
}

uint64_t rng_next(struct rng * r) {
    uint64_t x = r->state;
    x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
    r->state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

float rng_uniform(struct rng * r) {
    return (float)((rng_next(r) >> 40) & 0xFFFFFF) / (float)(1u << 24);
}

#endif // core_implementation

