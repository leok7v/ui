#include "rt.h"
#include <stdatomic.h> // needs cl.exe /experimental:c11atomics command line

// see: https://developercommunity.visualstudio.com/t/C11--C17-include-stdatomich-issue/10620622

#define ATOMICS_HAS_STDATOMIC_H

#ifndef __INTELLISENSE__ // IntelliSense chokes on _Atomic(_Type) etc...

// __INTELLISENSE__ Defined as 1 during an IntelliSense compiler pass
// in the Visual Studio IDE. Otherwise, undefined. You can use this macro
// to guard code the IntelliSense compiler doesn't understand,
// or use it to toggle between the build and IntelliSense compiler.

static_assertion(sizeof(void*) == sizeof(int64_t));
static_assertion(sizeof(void*) == sizeof(uintptr_t));

#ifndef ATOMICS_HAS_STDATOMIC_H

static int32_t atomics_increment_int32(volatile int32_t* a) {
    return InterlockedIncrement((volatile LONG*)a);
}

static int32_t atomics_decrement_int32(volatile int32_t* a) {
    return InterlockedDecrement((volatile LONG*)a);
}

static int64_t atomics_increment_int64(volatile int64_t* a) {
    return InterlockedIncrement64((__int64 volatile *)a);
}

static int64_t atomics_decrement_int64(volatile int64_t* a) {
    return InterlockedDecrement64((__int64 volatile *)a);
}

static int32_t atomics_add_int32(volatile int32_t* a, int32_t v) {
    return InterlockedAdd((LONG volatile *)a, v);
}

static int64_t atomics_add_int64(volatile int64_t* a, int64_t v) {
    return InterlockedAdd64((__int64 volatile *)a, v);
}

static int64_t atomics_exchange_int64(volatile int64_t* a, int64_t v) {
    return (int64_t)InterlockedExchange64((LONGLONG*)a, (LONGLONG)v);
}

static int32_t atomics_exchange_int32(volatile int32_t* a, int32_t v) {
    assert(sizeof(int32_t) == sizeof(unsigned long));
    return (int32_t)InterlockedExchange((volatile LONG*)a, (unsigned long)v);
}

static bool atomics_compare_exchange_int64(volatile int64_t* a,
        int64_t comparand, int64_t v) {
    return (int64_t)InterlockedCompareExchange64((LONGLONG*)a,
        (LONGLONG)v, (LONGLONG)comparand) == comparand;
}

static bool atomics_compare_exchange_int32(volatile int32_t* a,
        int32_t comparand, int32_t v) {
    return (int64_t)InterlockedCompareExchange((LONG*)a,
        (LONG)v, (LONG)comparand) == comparand;
}

static void memory_fence(void) { _mm_mfence(); }

#else

// stdatomic.h version:

// _strong() operations are the same as _explicit(..., memory_order_seq_cst)
// memory_order_seq_cst stands for Sequentially Consistent Ordering
//
// This is the strongest memory order, providing the guarantee that
// all sequentially consistent operations appear to be executed in
// the same order on all threads (cores)
//
// int_fast32_t: Fastest integer type with at least 32 bits.
// int_least32_t: Smallest integer type with at least 32 bits.

static_assertion(sizeof(int32_t) == sizeof(int_fast32_t));
static_assertion(sizeof(int32_t) == sizeof(int_least32_t));

static int32_t atomics_increment_int32(volatile int32_t* a) {
    return atomic_fetch_add((atomic_int_fast32_t*)a, 1) + 1;
}

static int32_t atomics_decrement_int32(volatile int32_t* a) {
    return atomic_fetch_sub((atomic_int_fast32_t*)a, 1) - 1;
}

static int64_t atomics_increment_int64(volatile int64_t* a) {
    return atomic_fetch_add((atomic_int_fast64_t*)a, 1) + 1;
}

static int64_t atomics_decrement_int64(volatile int64_t* a) {
    return atomic_fetch_sub((atomic_int_fast64_t*)a, 1) - 1;
}

static int32_t atomics_add_int32(volatile int32_t* a, int32_t v) {
    return atomic_fetch_add((atomic_int_fast32_t*)a, v) + v;
}

static int64_t atomics_add_int64(volatile int64_t* a, int64_t v) {
    return atomic_fetch_add((atomic_int_fast64_t*)a, v) + v;
}

static int64_t atomics_exchange_int64(volatile int64_t* a, int64_t v) {
    return atomic_exchange((atomic_int_fast64_t*)a, v);
}

static int32_t atomics_exchange_int32(volatile int32_t* a, int32_t v) {
    return atomic_exchange((atomic_int_fast32_t*)a, v);
}

static bool atomics_compare_exchange_int64(volatile int64_t* a,
    int64_t comparand, int64_t v) {
    return atomic_compare_exchange_strong((atomic_int_fast64_t*)a,
        &comparand, v);
}

// Code here is not "seen" by IntelliSense but is compiled normally.
static bool atomics_compare_exchange_int32(volatile int32_t* a,
    int32_t comparand, int32_t v) {
    return atomic_compare_exchange_strong((atomic_int_fast32_t*)a,
        &comparand, v);
}

static void memory_fence(void) { atomic_thread_fence(memory_order_seq_cst); }

#endif // ATOMICS_HAS_STDATOMIC_H

static int32_t atomics_load_int32(volatile int32_t* a) {
    return atomics.add_int32(a, 0);
}

static int64_t atomics_load_int64(volatile int64_t* a) {
    return atomics.add_int64(a, 0);
}

static void* atomics_exchange_ptr(volatile void** a, void* v) {
    static_assertion(sizeof(void*) == sizeof(uint64_t));
    return (void*)atomics.exchange_int64((int64_t*)a, (int64_t)v);
}

static bool atomics_compare_exchange_ptr(volatile void* *a, void* comparand, void* v) {
    static_assertion(sizeof(void*) == sizeof(int64_t));
    return atomics.compare_exchange_int64((int64_t*)a,
        (int64_t)v, (int64_t)comparand);
}

// https://en.wikipedia.org/wiki/Spinlock

#define __sync_bool_compare_and_swap(p, old_val, new_val) \
    _InterlockedCompareExchange64(p, new_val, old_val) == old_val

// https://stackoverflow.com/questions/37063700/mm-pause-usage-in-gcc-on-intel
#define __builtin_cpu_pause() YieldProcessor()

static void spinlock_acquire(volatile int64_t* spinlock) {
    // Very basic implementation of a spinlock. This is currently
    // only used to guarantee thread-safety during context initialization
    // and shutdown (which are both executed very infrequently and
    // have minimal thread contention).
    // Not a performance champion (because of mem_fence()) but serves
    // the purpose. mem_fence() can be reduced to mem_sfence()... sigh
    while (!__sync_bool_compare_and_swap(spinlock, 0, 1)) {
        while (*spinlock) {
            __builtin_cpu_pause();
        }
    }
    atomics.memory_fence();
    // not strcitly necessary on strong mem model Intel/AMD but
    // see: https://cfsamsonbooks.gitbook.io/explaining-atomics-in-rust/
    //      Fig 2 Inconsistent C11 execution of SB and 2+2W
    assert(*spinlock == 1);
}

static void spinlock_release(volatile int64_t* spinlock) {
    assert(*spinlock == 1);
    *spinlock = 0;
    atomics.memory_fence(); // tribute to lengthy Linus discussion going since 2006
}

atomics_if atomics = {
    .exchange_ptr    = atomics_exchange_ptr,
    .increment_int32 = atomics_increment_int32,
    .decrement_int32 = atomics_decrement_int32,
    .increment_int64 = atomics_increment_int64,
    .decrement_int64 = atomics_decrement_int64,
    .add_int32 = atomics_add_int32,
    .add_int64 = atomics_add_int64,
    .exchange_int32  = atomics_exchange_int32,
    .exchange_int64  = atomics_exchange_int64,
    .compare_exchange_int64 = atomics_compare_exchange_int64,
    .compare_exchange_int32 = atomics_compare_exchange_int32,
    .compare_exchange_ptr = atomics_compare_exchange_ptr,
    .load32 = atomics_load_int32,
    .load64 = atomics_load_int64,
    .spinlock_acquire = spinlock_acquire,
    .spinlock_release = spinlock_release,
    .memory_fence = memory_fence
};

#endif // __INTELLISENSE__

// 2024-03-20 latest windows runtime and toolchain cl.exe
// ... VC\Tools\MSVC\14.39.33519\include
// see:
//     vcruntime_c11_atomic_support.h
//     vcruntime_c11_stdatomic.h
//     stdatomic.h
// https://developercommunity.visualstudio.com/t/C11--C17-include--issue/10620622
// cl.exe /std:c11 /experimental:c11atomics
// command line option are required
// even in C17 mode in spring of 2024
