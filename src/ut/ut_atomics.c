#include "ut/ut.h"
#include "ut/ut_win32.h"
#include <stdatomic.h> // needs cl.exe /experimental:c11atomics command line

// see: https://developercommunity.visualstudio.com/t/C11--C17-include-stdatomich-issue/10620622

#pragma warning(push)
#pragma warning(disable: 4746) // volatile access of 'int32_var' is subject to /volatile:<iso|ms> setting; consider using __iso_volatile_load/store intrinsic functions

#ifndef UT_ATOMICS_HAS_STDATOMIC_H

static int32_t ut_atomics_increment_int32(volatile int32_t* a) {
    return InterlockedIncrement((volatile LONG*)a);
}

static int32_t ut_atomics_decrement_int32(volatile int32_t* a) {
    return InterlockedDecrement((volatile LONG*)a);
}

static int64_t ut_atomics_increment_int64(volatile int64_t* a) {
    return InterlockedIncrement64((__int64 volatile *)a);
}

static int64_t ut_atomics_decrement_int64(volatile int64_t* a) {
    return InterlockedDecrement64((__int64 volatile *)a);
}

static int32_t ut_atomics_add_int32(volatile int32_t* a, int32_t v) {
    return InterlockedAdd((LONG volatile *)a, v);
}

static int64_t ut_atomics_add_int64(volatile int64_t* a, int64_t v) {
    return InterlockedAdd64((__int64 volatile *)a, v);
}

static int64_t ut_atomics_exchange_int64(volatile int64_t* a, int64_t v) {
    return (int64_t)InterlockedExchange64((LONGLONG*)a, (LONGLONG)v);
}

static int32_t ut_atomics_exchange_int32(volatile int32_t* a, int32_t v) {
    ut_assert(sizeof(int32_t) == sizeof(unsigned long));
    return (int32_t)InterlockedExchange((volatile LONG*)a, (unsigned long)v);
}

static bool ut_atomics_compare_exchange_int64(volatile int64_t* a,
        int64_t comparand, int64_t v) {
    return (int64_t)InterlockedCompareExchange64((LONGLONG*)a,
        (LONGLONG)v, (LONGLONG)comparand) == comparand;
}

static bool ut_atomics_compare_exchange_int32(volatile int32_t* a,
        int32_t comparand, int32_t v) {
    return (int64_t)InterlockedCompareExchange((LONG*)a,
        (LONG)v, (LONG)comparand) == comparand;
}

static void memory_fence(void) {
#ifdef _M_ARM64
atomic_thread_fence(memory_order_seq_cst);
#else
_mm_mfence();
#endif
}

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

ut_static_assertion(sizeof(int32_t) == sizeof(int_fast32_t));
ut_static_assertion(sizeof(int32_t) == sizeof(int_least32_t));

static int32_t ut_atomics_increment_int32(volatile int32_t* a) {
    return atomic_fetch_add((volatile atomic_int_fast32_t*)a, 1) + 1;
}

static int32_t ut_atomics_decrement_int32(volatile int32_t* a) {
    return atomic_fetch_sub((volatile atomic_int_fast32_t*)a, 1) - 1;
}

static int64_t ut_atomics_increment_int64(volatile int64_t* a) {
    return atomic_fetch_add((volatile atomic_int_fast64_t*)a, 1) + 1;
}

static int64_t ut_atomics_decrement_int64(volatile int64_t* a) {
    return atomic_fetch_sub((volatile atomic_int_fast64_t*)a, 1) - 1;
}

static int32_t ut_atomics_add_int32(volatile int32_t* a, int32_t v) {
    return atomic_fetch_add((volatile atomic_int_fast32_t*)a, v) + v;
}

static int64_t ut_atomics_add_int64(volatile int64_t* a, int64_t v) {
    return atomic_fetch_add((volatile atomic_int_fast64_t*)a, v) + v;
}

static int64_t ut_atomics_exchange_int64(volatile int64_t* a, int64_t v) {
    return atomic_exchange((volatile atomic_int_fast64_t*)a, v);
}

static int32_t ut_atomics_exchange_int32(volatile int32_t* a, int32_t v) {
    return atomic_exchange((volatile atomic_int_fast32_t*)a, v);
}

static bool ut_atomics_compare_exchange_int64(volatile int64_t* a,
    int64_t comparand, int64_t v) {
    return atomic_compare_exchange_strong((volatile atomic_int_fast64_t*)a,
        &comparand, v);
}

// Code here is not "seen" by IntelliSense but is compiled normally.
static bool ut_atomics_compare_exchange_int32(volatile int32_t* a,
    int32_t comparand, int32_t v) {
    return atomic_compare_exchange_strong((volatile atomic_int_fast32_t*)a,
        &comparand, v);
}

static void memory_fence(void) { atomic_thread_fence(memory_order_seq_cst); }

#endif // __INTELLISENSE__

#endif // UT_ATOMICS_HAS_STDATOMIC_H

static int32_t ut_atomics_load_int32(volatile int32_t* a) {
    return ut_atomics.add_int32(a, 0);
}

static int64_t ut_atomics_load_int64(volatile int64_t* a) {
    return ut_atomics.add_int64(a, 0);
}

static void* ut_atomics_exchange_ptr(volatile void* *a, void* v) {
    ut_static_assertion(sizeof(void*) == sizeof(uint64_t));
    return (void*)(intptr_t)ut_atomics.exchange_int64((int64_t*)a, (int64_t)v);
}

static bool ut_atomics_compare_exchange_ptr(volatile void* *a, void* comparand, void* v) {
    ut_static_assertion(sizeof(void*) == sizeof(int64_t));
    return ut_atomics.compare_exchange_int64((int64_t*)a,
        (int64_t)comparand, (int64_t)v);
}

#pragma push_macro("ut_sync_bool_compare_and_swap")
#pragma push_macro("ut_builtin_cpu_pause")

// https://en.wikipedia.org/wiki/Spinlock

#define ut_sync_bool_compare_and_swap(p, old_val, new_val)          \
    (_InterlockedCompareExchange64(p, new_val, old_val) == old_val)

// https://stackoverflow.com/questions/37063700/mm-pause-usage-in-gcc-on-intel
#define ut_builtin_cpu_pause() do { YieldProcessor(); } while (0)

static void spinlock_acquire(volatile int64_t* spinlock) {
    // Very basic implementation of a spinlock. This is currently
    // only used to guarantee thread-safety during context initialization
    // and shutdown (which are both executed very infrequently and
    // have minimal thread contention).
    // Not a performance champion (because of mem_fence()) but serves
    // the purpose. mem_fence() can be reduced to mem_sfence()... sigh
    while (!ut_sync_bool_compare_and_swap(spinlock, 0, 1)) {
        while (*spinlock) { ut_builtin_cpu_pause(); }
    }
    ut_atomics.memory_fence();
    // not strictly necessary on strong mem model Intel/AMD but
    // see: https://cfsamsonbooks.gitbook.io/explaining-atomics-in-rust/
    //      Fig 2 Inconsistent C11 execution of SB and 2+2W
    ut_assert(*spinlock == 1);
}

#pragma pop_macro("ut_builtin_cpu_pause")
#pragma pop_macro("ut_sync_bool_compare_and_swap")

static void spinlock_release(volatile int64_t* spinlock) {
    ut_assert(*spinlock == 1);
    *spinlock = 0;
    // tribute to lengthy Linus discussion going since 2006:
    ut_atomics.memory_fence();
}

static void ut_atomics_test(void) {
    #ifdef UT_TESTS
    volatile int32_t int32_var = 0;
    volatile int64_t int64_var = 0;
    volatile void* ptr_var = null;
    int64_t spinlock = 0;
    void* old_ptr = ut_atomics.exchange_ptr(&ptr_var, (void*)123);
    ut_swear(old_ptr == null);
    ut_swear(ptr_var == (void*)123);
    int32_t incremented_int32 = ut_atomics.increment_int32(&int32_var);
    ut_swear(incremented_int32 == 1);
    ut_swear(int32_var == 1);
    int32_t decremented_int32 = ut_atomics.decrement_int32(&int32_var);
    ut_swear(decremented_int32 == 0);
    ut_swear(int32_var == 0);
    int64_t incremented_int64 = ut_atomics.increment_int64(&int64_var);
    ut_swear(incremented_int64 == 1);
    ut_swear(int64_var == 1);
    int64_t decremented_int64 = ut_atomics.decrement_int64(&int64_var);
    ut_swear(decremented_int64 == 0);
    ut_swear(int64_var == 0);
    int32_t added_int32 = ut_atomics.add_int32(&int32_var, 5);
    ut_swear(added_int32 == 5);
    ut_swear(int32_var == 5);
    int64_t added_int64 = ut_atomics.add_int64(&int64_var, 10);
    ut_swear(added_int64 == 10);
    ut_swear(int64_var == 10);
    int32_t old_int32 = ut_atomics.exchange_int32(&int32_var, 3);
    ut_swear(old_int32 == 5);
    ut_swear(int32_var == 3);
    int64_t old_int64 = ut_atomics.exchange_int64(&int64_var, 6);
    ut_swear(old_int64 == 10);
    ut_swear(int64_var == 6);
    bool int32_exchanged = ut_atomics.compare_exchange_int32(&int32_var, 3, 4);
    ut_swear(int32_exchanged);
    ut_swear(int32_var == 4);
    bool int64_exchanged = ut_atomics.compare_exchange_int64(&int64_var, 6, 7);
    ut_swear(int64_exchanged);
    ut_swear(int64_var == 7);
    ptr_var = (void*)0x123;
    bool ptr_exchanged = ut_atomics.compare_exchange_ptr(&ptr_var,
        (void*)0x123, (void*)0x456);
    ut_swear(ptr_exchanged);
    ut_swear(ptr_var == (void*)0x456);
    ut_atomics.spinlock_acquire(&spinlock);
    ut_swear(spinlock == 1);
    ut_atomics.spinlock_release(&spinlock);
    ut_swear(spinlock == 0);
    int32_t loaded_int32 = ut_atomics.load32(&int32_var);
    ut_swear(loaded_int32 == int32_var);
    int64_t loaded_int64 = ut_atomics.load64(&int64_var);
    ut_swear(loaded_int64 == int64_var);
    ut_atomics.memory_fence();
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_println("done"); }
    #endif
}

#ifndef __INTELLISENSE__ // IntelliSense chokes on _Atomic(_Type)

ut_static_assertion(sizeof(void*) == sizeof(int64_t));
ut_static_assertion(sizeof(void*) == sizeof(uintptr_t));

ut_atomics_if ut_atomics = {
    .exchange_ptr    = ut_atomics_exchange_ptr,
    .increment_int32 = ut_atomics_increment_int32,
    .decrement_int32 = ut_atomics_decrement_int32,
    .increment_int64 = ut_atomics_increment_int64,
    .decrement_int64 = ut_atomics_decrement_int64,
    .add_int32 = ut_atomics_add_int32,
    .add_int64 = ut_atomics_add_int64,
    .exchange_int32  = ut_atomics_exchange_int32,
    .exchange_int64  = ut_atomics_exchange_int64,
    .compare_exchange_int64 = ut_atomics_compare_exchange_int64,
    .compare_exchange_int32 = ut_atomics_compare_exchange_int32,
    .compare_exchange_ptr = ut_atomics_compare_exchange_ptr,
    .load32 = ut_atomics_load_int32,
    .load64 = ut_atomics_load_int64,
    .spinlock_acquire = spinlock_acquire,
    .spinlock_release = spinlock_release,
    .memory_fence = memory_fence,
    .test = ut_atomics_test
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

#pragma warning(pop)
