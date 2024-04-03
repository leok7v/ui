#include "runtime.h"
#include "win32.h"
#include <stdatomic.h> // needs cl.exe /experimental:c11atomics command line

// see: https://developercommunity.visualstudio.com/t/C11--C17-include-stdatomich-issue/10620622

#define ATOMICS_HAS_STDATOMIC_H

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

#ifndef __INTELLISENSE__ // IntelliSense chokes on _Atomic(_Type)
// __INTELLISENSE__ Defined as 1 during an IntelliSense compiler pass
// in the Visual Studio IDE. Otherwise, undefined. You can use this macro
// to guard code the IntelliSense compiler doesn't understand,
// or use it to toggle between the build and IntelliSense compiler.


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

#endif // __INTELLISENSE__

#endif // ATOMICS_HAS_STDATOMIC_H

static int32_t atomics_load_int32(volatile int32_t* a) {
    return atomics.add_int32(a, 0);
}

static int64_t atomics_load_int64(volatile int64_t* a) {
    return atomics.add_int64(a, 0);
}

static void* atomics_exchange_ptr(volatile void* *a, void* v) {
    static_assertion(sizeof(void*) == sizeof(uint64_t));
    return (void*)atomics.exchange_int64((int64_t*)a, (int64_t)v);
}

static bool atomics_compare_exchange_ptr(volatile void* *a, void* comparand, void* v) {
    static_assertion(sizeof(void*) == sizeof(int64_t));
    return atomics.compare_exchange_int64((int64_t*)a,
        (int64_t)comparand, (int64_t)v);
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
    // tribute to lengthy Linus discussion going since 2006:
    atomics.memory_fence();
}

static void atomics_test(void) {
    #ifdef RUNTIME_TESTS
    volatile int32_t int32_var = 0;
    volatile int64_t int64_var = 0;
    volatile void* ptr_var = null;
    int64_t spinlock = 0;
    void* old_ptr = atomics.exchange_ptr(&ptr_var, (void*)123);
    swear(old_ptr == null);
    swear(ptr_var == (void*)123);
    int32_t incremented_int32 = atomics.increment_int32(&int32_var);
    swear(incremented_int32 == 1);
    swear(int32_var == 1);
    int32_t decremented_int32 = atomics.decrement_int32(&int32_var);
    swear(decremented_int32 == 0);
    swear(int32_var == 0);
    int64_t incremented_int64 = atomics.increment_int64(&int64_var);
    swear(incremented_int64 == 1);
    swear(int64_var == 1);
    int64_t decremented_int64 = atomics.decrement_int64(&int64_var);
    swear(decremented_int64 == 0);
    swear(int64_var == 0);
    int32_t added_int32 = atomics.add_int32(&int32_var, 5);
    swear(added_int32 == 5);
    swear(int32_var == 5);
    int64_t added_int64 = atomics.add_int64(&int64_var, 10);
    swear(added_int64 == 10);
    swear(int64_var == 10);
    int32_t old_int32 = atomics.exchange_int32(&int32_var, 3);
    swear(old_int32 == 5);
    swear(int32_var == 3);
    int64_t old_int64 = atomics.exchange_int64(&int64_var, 6);
    swear(old_int64 == 10);
    swear(int64_var == 6);
    bool int32_exchanged = atomics.compare_exchange_int32(&int32_var, 3, 4);
    swear(int32_exchanged);
    swear(int32_var == 4);
    bool int64_exchanged = atomics.compare_exchange_int64(&int64_var, 6, 7);
    swear(int64_exchanged);
    swear(int64_var == 7);
    ptr_var = (void*)0x123;
    bool ptr_exchanged = atomics.compare_exchange_ptr(&ptr_var,
        (void*)0x123, (void*)0x456);
    swear(ptr_exchanged);
    swear(ptr_var == (void*)0x456);
    atomics.spinlock_acquire(&spinlock);
    swear(spinlock == 1);
    atomics.spinlock_release(&spinlock);
    swear(spinlock == 0);
    int32_t loaded_int32 = atomics.load32(&int32_var);
    swear(loaded_int32 == int32_var);
    int64_t loaded_int64 = atomics.load64(&int64_var);
    swear(loaded_int64 == int64_var);
    atomics.memory_fence();
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
    #endif
}

#ifndef __INTELLISENSE__ // IntelliSense chokes on _Atomic(_Type)

static_assertion(sizeof(void*) == sizeof(int64_t));
static_assertion(sizeof(void*) == sizeof(uintptr_t));

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
    .memory_fence = memory_fence,
    .test = atomics_test
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
