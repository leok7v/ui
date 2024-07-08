#pragma once
#include "ut/ut.h"

begin_c

typedef struct ut_react_call_s ut_react_call_t;
typedef struct ut_react_queue_s ut_react_queue_t;

typedef struct ut_react_call_s {
    void (*proc)(ut_react_call_t* c);
    void*  data;   // extra data that will be passed to proc() call
    fp64_t time;  // proc(data) call will be made after or at this time
    bool   canceled; // := true on cancel() call
    ut_react_queue_t* queue; // queue where the call is or was last scheduled
    ut_react_call_t*  next;  // next element in the queue (implementation detail)
} ut_react_call_t;

typedef struct ut_react_queue_s {
    int64_t lock; // spinlock
    ut_react_call_t* head;
} ut_react_queue_t;

typedef struct ut_react_if {
    void (*dispatch)(ut_react_queue_t* q, fp64_t now);
    void (*enqueue)(ut_react_queue_t* q, ut_react_call_t* c, fp64_t time);
    void (*cancel)(ut_react_call_t* c);
    void (*flush)(ut_react_queue_t* q); // cancel all requests in the queue
    void (*test)(void);
} ut_react_if;

extern ut_react_if ut_react;

end_c

/*
    Usage examples:

    // The dispatch_for() is just for testing purposes.
    // Usually ut_react.dispatch(q) will be called inside each
    // iteration of message loop of a dispatch [UI] thread.

    static void dispatch_for(ut_react_queue_t* q, fp64_t seconds);

    // simple way of passing a single pointer to call_later

    static void every_second(ut_react_call_t* c) {
        int32_t* i = (int32_t*)c->data;
        traceln("%d", *i);
        (*i)++;
        ut_react.enqueue(c->queue, c, ut_clock.seconds() + 1.0);
    }

    static void example_1(void) {
        ut_react_queue_t q = {0};
        // if a single pointer will suffice
        int32_t i = 0;
        ut_react_call_t c = {.proc = every_second, .data = &i };
        ut_react.enqueue(&q, &c, ut_clock.seconds() + 1.0);
        dispatch_for(&q, 3.0);
    }

    // extending ut_react_call_t with extra data:

    typedef struct ut_call_ex_s {
        union {
            ut_react_call_t base;
            struct ut_react_call_s;
        };
        struct { int32_t a; int32_t b; } s;
        int32_t i;
    } ut_call_ex_t;

    static void every_other_second(ut_react_call_t* c) {
        ut_call_ex_t* ex = (ut_call_ex_t*)c;
        traceln(".i: %d .extra: {.a: %d .b: %d} now: %.6f time: %.6f",
                ex->i, ex->s.a, ex->s.b,
                ut_clock.seconds(), c->time);
        ex->i++;
        const int32_t swap = ex->s.a; ex->s.a = ex->s.b; ex->s.b = swap;
        ut_react.enqueue(c->queue, c, ut_clock.seconds() + 2.0);
    }

    static void example_2(void) {
        ut_react_queue_t q = {0};
        ut_call_ex_t ex = {
            .proc = every_other_second,
            .s = { .a = 1, .b = 2 },
            .i = 0
        };
        ut_react.enqueue(&q, &ex.base, ut_clock.seconds() + 2.0);
        dispatch_for(&q, 6.0);
    }

    static void dispatch_for(ut_react_queue_t* q, fp64_t seconds) {
        fp64_t deadline = ut_clock.seconds() + seconds;
        while (q->head != null && ut_clock.seconds() < deadline) {
            ut_thread.sleep_for(0.0001); // 100 microseconds
            ut_react.dispatch(q, ut_clock.seconds());
        }
        ut_react.flush(q);
    }

*/
