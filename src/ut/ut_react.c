#include "ut/ut.h"

static void ut_react_check_for_duplicates(ut_react_queue_t* q,
        ut_react_call_t* c) {
    ut_react_call_t* e = q->head;
    bool found = false;
    while (e != null && !found) {
        found = e == c;
        if (!found) { e = e->next; }
    }
    swear(!found);
}

static void ut_react_enqueue(ut_react_queue_t* q, ut_react_call_t* c, fp64_t t) {
    assert(c->proc != null && t >= 0.0 && q != null && c != null);
    ut_atomics.spinlock_acquire(&q->lock);
    ut_react_check_for_duplicates(q, c);
    c->queue = q;
    c->time = t;
    //  Enqueue in time sorted order least ->time first to save
    //  time searching in fetching from queue which is more frequent.
    if (q->head == null || q->head->time > t) {
        c->next = q->head;
        q->head = c;
    } else {
        ut_react_call_t* p = null;
        ut_react_call_t* e = q->head;
        while (e != null && e->time <= t) {
            p = e;
            e = e->next;
        }
        c->next = e;
        if (p == null) { q->head = c; } else { p->next = c; }
    }
    ut_atomics.spinlock_release(&q->lock);
}

static void ut_react_cancel(ut_react_call_t* c) {
    swear(!c->canceled && c->queue != null);
    ut_react_queue_t* q = c->queue;
    ut_atomics.spinlock_acquire(&q->lock);
    if (q->head != null) {
        ut_react_call_t* p = null;
        ut_react_call_t* e = q->head;
        while (e != null && !c->canceled) {
            if (e == c) {
                if (p == null) {
                    q->head = e->next;
                } else {
                    p->next = e->next;
                }
                e->next = null;
                e->canceled = true;
            } else {
                p = e;
                e = e->next;
            }
        }
    }
    ut_atomics.spinlock_release(&q->lock);
    swear(c->canceled);
}

static void ut_react_flush(ut_react_queue_t* q) {
    while (q->head != null) { ut_react.cancel(q->head); }
}

static ut_react_call_t* ut_react_dequeue(ut_react_queue_t* q, fp64_t now) {
    ut_react_call_t* c = null;
    ut_atomics.spinlock_acquire(&q->lock);
    if (q->head != null && q->head->time <= now) {
        c = q->head;
        q->head = c->next;
        c->next = null;
    }
    ut_atomics.spinlock_release(&q->lock);
    return c;
}

static void ut_react_dispatch(ut_react_queue_t* q, fp64_t now) {
    ut_react_call_t* c = ut_react_dequeue(q, now);
    while (c != null) {
        assert(now >= c->time);
        c->proc(c);
        c = ut_react_dequeue(q, now);
    }
}

#ifdef UT_TESTS

// tests:

// keep in mind that traceln() itself is subject of "astronomical"
// wait state times in order of dozens of milliseconds.

static int32_t ut_react_called;

static void ut_react_never_called(ut_react_call_t* unused(c)) {
    ut_react_called++;
}

static void ut_react_test_1(void) {
    ut_react_called = 0;
    // testing insertion time ordering of two events into queue
    ut_react_queue_t q = {0};
    ut_react_call_t c1 = {.proc = ut_react_never_called};
    ut_react_call_t c2 = {.proc = ut_react_never_called};
    ut_react.enqueue(&q, &c1, 1.0);
    swear(q.head == &c1 && q.head->next == null);
    ut_react.enqueue(&q, &c2, 0.5);
    swear(q.head == &c2 && q.head->next == &c1);
    ut_react.flush(&q);
    // test that canceled events are not dispatched
    swear(ut_react_called == 0 && c1.canceled && c2.canceled && q.head == null);
    c1.canceled = false;
    c2.canceled = false;
    // test the ut_react.cancel() function
    ut_react.enqueue(&q, &c1, 1.0);
    ut_react.enqueue(&q, &c2, 0.5);
    swear(q.head == &c2 && q.head->next == &c1);
    ut_react.cancel(&c2);
    swear(c2.canceled && q.head == &c1 && q.head->next == null);
    c2.canceled = false;
    ut_react.enqueue(&q, &c2, 1.0);
    ut_react.cancel(&c1);
    swear(c1.canceled && q.head == &c2 && q.head->next == null);
    ut_react.flush(&q);
    swear(ut_react_called == 0 && c1.canceled && c2.canceled && q.head == null);
}

// simple way of passing a single pointer to call_later

static fp64_t ut_react_t0; // makes timinng a bit easier to read

static void ut_react_every_millisecond(ut_react_call_t* c) {
    int32_t* i = (int32_t*)c->data;
    if (ut_debug.verbosity.level > ut_debug.verbosity.info) {
        traceln("%d now: %.6f time: %.6f", *i,
                ut_clock.seconds() - ut_react_t0, c->time - ut_react_t0);
    }
    (*i)++;
    ut_react.enqueue(c->queue, c, ut_clock.seconds() + 0.001);
}

static void ut_react_test_2(void) {
    ut_thread.realtime();
    ut_react_t0 = ut_clock.seconds();
    ut_react_queue_t q = {0};
    // if a single pointer will suffice
    int32_t i = 0;
    ut_react_call_t c = {.proc = ut_react_every_millisecond, .data = &i };
    ut_react.enqueue(&q, &c, ut_clock.seconds() + 0.001);
    fp64_t deadline = ut_clock.seconds() + 0.010;
    while (q.head != null && ut_clock.seconds() < deadline) {
        ut_thread.sleep_for(0.0001); // 100 microseconds
        ut_react.dispatch(&q, ut_clock.seconds());
    }
    ut_react.flush(&q);
    swear(q.head == null);
    swear(i > 0);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) {
        traceln("called: %d times", i);
    }
}

// extending ut_react_call_t with extra data:

typedef struct ut_call_ex_s {
    // nameless union opens up base fields into ut_call_ex_t
    // it is not necessary at all
    union {
        ut_react_call_t base;
        struct ut_react_call_s;
    };
    struct { int32_t a; int32_t b; } s;
    int32_t i;
} ut_call_ex_t;

static void ut_react_every_other_millisecond(ut_react_call_t* c) {
    ut_call_ex_t* ex = (ut_call_ex_t*)c;
    if (ut_debug.verbosity.level > ut_debug.verbosity.info) {
        traceln(".i: %d .extra: {.a: %d .b: %d} now: %.6f time: %.6f",
                ex->i, ex->s.a, ex->s.b,
                ut_clock.seconds() - ut_react_t0, c->time - ut_react_t0);
    }
    ex->i++;
    const int32_t swap = ex->s.a; ex->s.a = ex->s.b; ex->s.b = swap;
    ut_react.enqueue(c->queue, c, ut_clock.seconds() + 0.002);
}

static void ut_react_test_3(void) {
    ut_thread.realtime();
    ut_static_assertion(offsetof(ut_call_ex_t, base) == 0);
    ut_react_queue_t q = {0};
    ut_call_ex_t ex = {
        .proc = ut_react_every_other_millisecond,
        .s = { .a = 1, .b = 2 },
        .i = 0
    };
    ut_react.enqueue(&q, &ex.base, ut_clock.seconds() + 0.002);
    fp64_t deadline = ut_clock.seconds() + 0.020;
    while (q.head != null && ut_clock.seconds() < deadline) {
        ut_thread.sleep_for(0.0001); // 100 microseconds
        ut_react.dispatch(&q, ut_clock.seconds());
    }
    ut_react.flush(&q);
    swear(q.head == null);
    swear(ex.i > 0);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) {
        traceln("called: %d times", ex.i);
    }
}

static void ut_react_test(void) {
//  uncomment one of the following lines to see the output
//  ut_debug.verbosity.level = ut_debug.verbosity.info;
//  ut_debug.verbosity.level = ut_debug.verbosity.verbose;
    ut_react_test_1();
    ut_react_test_2();
    ut_react_test_3();
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { traceln("done"); }
}

#else

static void ut_react_test(void) {}

#endif

ut_react_if ut_react = {
    .dispatch = ut_react_dispatch,
    .enqueue  = ut_react_enqueue,
    .cancel   = ut_react_cancel,
    .flush    = ut_react_flush,
    .test     = ut_react_test
};
