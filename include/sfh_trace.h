#ifndef trace_definition
#define trace_definition

// _________________________________ trace.h __________________________________

// Public surface of the trace ring + leveled logger.
// Implementation lives in src/trace.c. Standalone C (no rt_* dependency);
// compiles on Windows (MSVC /std:c17 /experimental:c11atomics), macOS and
// Linux.

#ifndef TRACE_H
#define TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TRACE_RING_CAPACITY 1024 // power of two; head % CAPACITY

enum trace_level {
    trace_level_debug = 0,
    trace_level_info  = 1,
    trace_level_warn  = 2,
    trace_level_error = 3,
};

// One ring slot. `file` / `function` point at literal __FILE__ / __func__
// in the read-only segment; `message` is heap-owned (NUL-terminated, no
// fixed cap). Slot reuse on wrap-around frees the prior `message` first.
struct trace_entry {
    double             timestamp; // seconds since first trace() call
    enum trace_level   level;
    const char *       file;      // basename (after last '/')
    const char *       function;  // __func__ at the call site
    int32_t            line;
    char *             message;   // owned; NUL-terminated; may be NULL
    size_t             message_n; // byte length excluding NUL
};

const char * trace_message(const struct trace_entry * e, size_t * out_n);

struct trace_observer {
    void * that;
    void (*on_trace)(const struct trace_observer * o,
                     const struct trace_entry * e);
};

#if defined(__GNUC__) || defined(__clang__)
#define TRACE_PRINTF_ATTR __attribute__((format(printf, 5, 6)))
#else
#define TRACE_PRINTF_ATTR
#endif

void _trace_(enum trace_level level,
             const char * filename, int32_t line, const char * func,
             const char * format, ...) TRACE_PRINTF_ATTR;

// Per-process minimum level for stderr mirroring. Ring captures
// everything regardless. Default trace_level_warn.
void             trace_set_min_level(enum trace_level lvl);
enum trace_level trace_min_level(void);

// trace(info, "loaded %d weights", n); — `level` is a bareword
// (debug | info | warn | error).
#define trace(level, format, ...) \
    _trace_(trace_level_##level, __FILE__, __LINE__, __func__, (format), ##__VA_ARGS__)

// abort() (not exit()) so we skip atexit/fflush of half-baked pipes;
// trace(error, ...) already flushed stderr.
#define fatal(format, ...) do { \
    trace(error, format, ##__VA_ARGS__); \
    abort(); \
} while (0)

// NULL = detach. Update is atomic; an in-flight trace() may fire the
// previous callback once more.
void trace_subscribe(const struct trace_observer * observer);

uint64_t trace_head(void);
const struct trace_entry * trace_at(uint64_t index);

#ifdef __cplusplus
}
#endif
#endif // TRACE_H

#endif // trace_definition

#if defined(trace_implementation) && !defined(trace_implementation_included)
#define trace_implementation_included
// _________________________________ trace.c __________________________________

// Trace ring + leveled logger -- implementation. See inc/trace.h.
// Standalone C; depends only on the C standard library plus the platform
// monotonic clock (QueryPerformanceCounter on Windows, clock_gettime on
// POSIX). No rt_* dependency.


#include <locale.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------------------------------------------------------------------
// monotonic time, seconds (double)
// ----------------------------------------------------------------------------

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

static double trace_seconds_now(void) {
    // Frequency is fixed for the life of the process; the unsynchronized
    // first-call init races only to write the same constant.
    static LARGE_INTEGER freq;
    if (freq.QuadPart == 0) { QueryPerformanceFrequency(&freq); }
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / (double)freq.QuadPart;
}

#else

#include <time.h>

static double trace_seconds_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

#endif

static double trace_since_start(void) {
    static double start_time = -1.0;
    double now = trace_seconds_now();
    if (start_time < 0.0) { start_time = now; }
    return now - start_time;
}

// Idempotent; first call enables `%'d` thousand-separator output where the
// platform's printf supports it (glibc/macOS; MSVC ignores the flag).
static void trace_set_numeric_locale(void) {
    static bool done = false;
    if (!done) {
        setlocale(LC_NUMERIC, "");
        setlocale(LC_NUMERIC, "en_US.UTF-8");
        done = true;
    }
}

// ----------------------------------------------------------------------------
// ring + logger
// ----------------------------------------------------------------------------

// Single-writer ring; release-store on g_trace_head makes entry contents
// visible to readers on other threads before the new head.
static struct trace_entry g_trace_ring[TRACE_RING_CAPACITY];
static _Atomic uint64_t   g_trace_head = 0;
static _Atomic int        g_trace_min_level = trace_level_error;
static _Atomic(struct trace_observer *) g_trace_observer = NULL;
static struct trace_observer g_trace_observer_slot;

void trace_set_min_level(enum trace_level lvl) {
    atomic_store_explicit(&g_trace_min_level, (int)lvl, memory_order_release);
}

enum trace_level trace_min_level(void) {
    return (enum trace_level)atomic_load_explicit(&g_trace_min_level,
                                                  memory_order_acquire);
}

static const char * trace_level_prefix(enum trace_level lvl) {
    const char * r = "?????";
    switch (lvl) {
        case trace_level_debug: r = "DEBUG"; break;
        case trace_level_info:  r = "INFO "; break;
        case trace_level_warn:  r = "WARN "; break;
        case trace_level_error: r = "ERROR"; break;
    }
    return r;
}

void _trace_(enum trace_level level,
             const char * filename, int32_t line, const char * func,
             const char * format, ...) {
    trace_set_numeric_locale();
    const char * file  = filename;
    const char * slash = strrchr(file, '/');
    if (slash != NULL) { file = slash + 1; }
    // also handle Windows path separators
    const char * back = strrchr(file, '\\');
    if (back != NULL) { file = back + 1; }
    uint64_t idx = atomic_load_explicit(&g_trace_head,
                                        memory_order_relaxed);
    struct trace_entry * e = &g_trace_ring[idx % TRACE_RING_CAPACITY];
    free(e->message);
    e->message   = NULL;
    e->message_n = 0;
    e->timestamp = trace_since_start();
    e->level     = level;
    e->file      = file;
    e->function  = func;
    e->line      = line;
    // Probe length, allocate exact, format. malloc failure here is silent
    // (trace must not abort the program); readers see NULL.
    va_list ap;
    va_start(ap, format);
    va_list cp;
    va_copy(cp, ap);
    int n = vsnprintf(NULL, 0, format, cp);
    va_end(cp);
    if (n > 0) {
        e->message = (char *)malloc((size_t)n + 1);
        if (e->message != NULL) {
            vsnprintf(e->message, (size_t)n + 1, format, ap);
            e->message_n = (size_t)n;
        }
    }
    va_end(ap);
    if ((int)level >= atomic_load_explicit(&g_trace_min_level,
                                           memory_order_acquire)) {
        const char * msg = (e->message != NULL) ? e->message : "";
        fprintf(stderr, "[%s] %s", trace_level_prefix(level), msg);
        if (e->message_n == 0 || e->message[e->message_n - 1] != '\n') {
            fputc('\n', stderr);
        }
    }
    atomic_store_explicit(&g_trace_head, idx + 1, memory_order_release);
    struct trace_observer * obs =
        atomic_load_explicit(&g_trace_observer, memory_order_acquire);
    if (obs != NULL && obs->on_trace != NULL) {
        obs->on_trace(obs, e);
    }
}

const char * trace_message(const struct trace_entry * e, size_t * out_n) {
    const char * r = NULL;
    if (e != NULL && e->message != NULL) {
        r = e->message;
        if (out_n != NULL) { *out_n = e->message_n; }
    } else if (out_n != NULL) {
        *out_n = 0;
    }
    return r;
}

void trace_subscribe(const struct trace_observer * observer) {
    if (observer == NULL) {
        atomic_store_explicit(&g_trace_observer, NULL,
                              memory_order_release);
    } else {
        g_trace_observer_slot = *observer;
        atomic_store_explicit(&g_trace_observer,
                              &g_trace_observer_slot,
                              memory_order_release);
    }
}

uint64_t trace_head(void) {
    return atomic_load_explicit(&g_trace_head, memory_order_acquire);
}

const struct trace_entry * trace_at(uint64_t index) {
    uint64_t head = atomic_load_explicit(&g_trace_head,
                                         memory_order_acquire);
    const struct trace_entry * r = NULL;
    if (index < head) {
        bool wrapped = head > TRACE_RING_CAPACITY
                    && index < head - TRACE_RING_CAPACITY;
        if (!wrapped) {
            r = &g_trace_ring[index % TRACE_RING_CAPACITY];
        }
    }
    return r;
}

#endif // trace_implementation

