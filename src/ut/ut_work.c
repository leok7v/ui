#include "ut/ut.h"

static void ut_work_queue_no_duplicates(ut_work_t* w) {
    ut_work_t* e = w->queue->head;
    bool found = false;
    while (e != null && !found) {
        found = e == w;
        if (!found) { e = e->next; }
    }
    swear(!found);
}

static void ut_work_queue_post(ut_work_t* w) {
    assert(w->queue != null && w != null && w->when >= 0.0);
    ut_work_queue_t* q = w->queue;
    ut_atomics.spinlock_acquire(&q->lock);
    ut_work_queue_no_duplicates(w); // under lock
    //  Enqueue in time sorted order least ->time first to save
    //  time searching in fetching from queue which is more frequent.
    ut_work_t* p = null;
    ut_work_t* e = q->head;
    while (e != null && e->when <= w->when) {
        p = e;
        e = e->next;
    }
    w->next = e;
    bool head = p == null;
    if (head) {
        q->head = w;
    } else {
        p->next = w;
    }
    ut_atomics.spinlock_release(&q->lock);
    if (head && q->changed != null) { ut_event.set(q->changed); }
}

static void ut_work_queue_cancel(ut_work_t* w) {
    swear(!w->canceled && w->queue != null && w->queue->head != null);
    ut_work_queue_t* q = w->queue;
    ut_atomics.spinlock_acquire(&q->lock);
    ut_work_t* p = null;
    ut_work_t* e = q->head;
    bool changed = false; // head changed
    while (e != null && !w->canceled) {
        if (e == w) {
            changed = p == null;
            if (changed) {
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
    ut_atomics.spinlock_release(&q->lock);
    swear(w->canceled);
    if (w->done != null) { ut_event.set(w->done); }
    if (changed && q->changed != null) { ut_event.set(q->changed); }
}

static void ut_work_queue_flush(ut_work_queue_t* q) {
    while (q->head != null) { ut_work_queue.cancel(q->head); }
}

static bool ut_work_queue_get(ut_work_queue_t* q, ut_work_t* *r) {
    ut_work_t* w = null;
    ut_atomics.spinlock_acquire(&q->lock);
    bool changed = q->head != null && q->head->when <= ut_clock.seconds();
    if (changed) {
        w = q->head;
        q->head = w->next;
        w->next = null;
    }
    ut_atomics.spinlock_release(&q->lock);
    *r = w;
    if (changed && q->changed != null) { ut_event.set(q->changed); }
    return w != null;
}

static void ut_work_queue_call(ut_work_t* w) {
    if (w->work != null) { w->work(w); }
    if (w->done != null) { ut_event.set(w->done); }
}

static void ut_work_queue_dispatch(ut_work_queue_t* q) {
    ut_work_t* w = null;
    while (ut_work_queue.get(q, &w)) { ut_work_queue.call(w); }
}

ut_work_queue_if ut_work_queue = {
    .post     = ut_work_queue_post,
    .get      = ut_work_queue_get,
    .call     = ut_work_queue_call,
    .dispatch = ut_work_queue_dispatch,
    .cancel   = ut_work_queue_cancel,
    .flush    = ut_work_queue_flush
};

static void ut_worker_thread(void* p) {
    ut_thread.name("worker");
    ut_worker_t* worker = (ut_worker_t*)p;
    ut_work_queue_t* q = &worker->queue;
    while (!worker->quit) {
        ut_work_queue.dispatch(q);
        fp64_t timeout = -1.0; // forever
        ut_atomics.spinlock_acquire(&q->lock);
        if (q->head != null) {
            timeout = ut_max(0, q->head->when - ut_clock.seconds());
        }
        ut_atomics.spinlock_release(&q->lock);
        // if another item is inserted into head after unlocking
        // the `wake` event guaranteed to be signalled
        if (!worker->quit && timeout != 0) {
            ut_event.wait_or_timeout(worker->wake, timeout);
        }
    }
    ut_work_queue.dispatch(q);
}

static void ut_worker_start(ut_worker_t* worker) {
    assert(worker->wake == null && !worker->quit);
    worker->wake  = ut_event.create();
    worker->queue = (ut_work_queue_t){
        .head = null, .lock = 0, .changed = worker->wake
    };
    worker->thread = ut_thread.start(ut_worker_thread, worker);
}

static errno_t ut_worker_join(ut_worker_t* worker, fp64_t to) {
    worker->quit = true;
    ut_event.set(worker->wake);
    errno_t r = ut_thread.join(worker->thread, to);
    if (r == 0) {
        ut_event.dispose(worker->wake);
        worker->wake = null;
        worker->thread = null;
        worker->quit = false;
        swear(worker->queue.head == null);
    }
    return r;
}

static void ut_worker_post(ut_worker_t* worker, ut_work_t* w) {
    assert(!worker->quit && worker->wake != null && worker->thread != null);
    w->queue = &worker->queue;
    ut_work_queue.post(w);
}

static void ut_worker_test(void);

ut_worker_if ut_worker = {
    .start = ut_worker_start,
    .post  = ut_worker_post,
    .join  = ut_worker_join,
    .test  = ut_worker_test
};

#ifdef UT_TESTS

// tests:

// keep in mind that ut_traceln() may be blocking and is a subject
// of "astronomical" wait state times in order of dozens of ms.

static int32_t ut_test_called;

static void ut_never_called(ut_work_t* ut_unused(w)) {
    ut_test_called++;
}

static void ut_work_queue_test_1(void) {
    ut_test_called = 0;
    // testing insertion time ordering of two events into queue
    const fp64_t now = ut_clock.seconds();
    ut_work_queue_t q = {0};
    ut_work_t c1 = {
        .queue = &q,
        .work = ut_never_called,
        .when = now + 1.0
    };
    ut_work_t c2 = {
        .queue = &q,
        .work = ut_never_called,
        .when = now + 0.5
    };
    ut_work_queue.post(&c1);
    swear(q.head == &c1 && q.head->next == null);
    ut_work_queue.post(&c2);
    swear(q.head == &c2 && q.head->next == &c1);
    ut_work_queue.flush(&q);
    // test that canceled events are not dispatched
    swear(ut_test_called == 0 && c1.canceled && c2.canceled && q.head == null);
    c1.canceled = false;
    c2.canceled = false;
    // test the ut_work_queue.cancel() function
    ut_work_queue.post(&c1);
    ut_work_queue.post(&c2);
    swear(q.head == &c2 && q.head->next == &c1);
    ut_work_queue.cancel(&c2);
    swear(c2.canceled && q.head == &c1 && q.head->next == null);
    c2.canceled = false;
    ut_work_queue.post(&c2);
    ut_work_queue.cancel(&c1);
    swear(c1.canceled && q.head == &c2 && q.head->next == null);
    ut_work_queue.flush(&q);
    swear(ut_test_called == 0 && c1.canceled && c2.canceled && q.head == null);
}

// simple way of passing a single pointer to call_later

static fp64_t ut_test_work_start; // makes timing debug traces easier to read

static void ut_every_millisecond(ut_work_t* w) {
    int32_t* i = (int32_t*)w->data;
    fp64_t now = ut_clock.seconds();
    if (ut_debug.verbosity.level > ut_debug.verbosity.info) {
        const fp64_t since_start = now - ut_test_work_start;
        const fp64_t dt = w->when - ut_test_work_start;
        ut_traceln("%d now: %.6f time: %.6f", *i, since_start, dt);
    }
    (*i)++;
    // read ut_clock.seconds() again because ut_traceln() above could block
    w->when = ut_clock.seconds() + 0.001;
    ut_work_queue.post(w);
}

static void ut_work_queue_test_2(void) {
    ut_thread.realtime();
    ut_test_work_start = ut_clock.seconds();
    ut_work_queue_t q = {0};
    // if a single pointer will suffice
    int32_t i = 0;
    ut_work_t c = {
        .queue = &q,
        .work = ut_every_millisecond,
        .when = ut_test_work_start + 0.001,
        .data = &i
    };
    ut_work_queue.post(&c);
    while (q.head != null && i < 8) {
        ut_thread.sleep_for(0.0001); // 100 microseconds
        ut_work_queue.dispatch(&q);
    }
    ut_work_queue.flush(&q);
    swear(q.head == null);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) {
        ut_traceln("called: %d times", i);
    }
}

// extending ut_work_t with extra data:

typedef struct ut_work_ex_s {
    // nameless union opens up base fields into ut_work_ex_t
    // it is not necessary at all
    union {
        ut_work_t base;
        struct ut_work_s;
    };
    struct { int32_t a; int32_t b; } s;
    int32_t i;
} ut_work_ex_t;

static void ut_every_other_millisecond(ut_work_t* w) {
    ut_work_ex_t* ex = (ut_work_ex_t*)w;
    fp64_t now = ut_clock.seconds();
    if (ut_debug.verbosity.level > ut_debug.verbosity.info) {
        const fp64_t since_start = now - ut_test_work_start;
        const fp64_t dt  = w->when - ut_test_work_start;
        ut_traceln(".i: %d .extra: {.a: %d .b: %d} now: %.6f time: %.6f",
                ex->i, ex->s.a, ex->s.b, since_start, dt);
    }
    ex->i++;
    const int32_t swap = ex->s.a; ex->s.a = ex->s.b; ex->s.b = swap;
    // read ut_clock.seconds() again because ut_traceln() above could block
    w->when = ut_clock.seconds() + 0.002;
    ut_work_queue.post(w);
}

static void ut_work_queue_test_3(void) {
    ut_thread.realtime();
    ut_static_assertion(offsetof(ut_work_ex_t, base) == 0);
    const fp64_t now = ut_clock.seconds();
    ut_work_queue_t q = {0};
    ut_work_ex_t ex = {
        .queue = &q,
        .work = ut_every_other_millisecond,
        .when = now + 0.002,
        .s = { .a = 1, .b = 2 },
        .i = 0
    };
    ut_work_queue.post(&ex.base);
    while (q.head != null && ex.i < 8) {
        ut_thread.sleep_for(0.0001); // 100 microseconds
        ut_work_queue.dispatch(&q);
    }
    ut_work_queue.flush(&q);
    swear(q.head == null);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) {
        ut_traceln("called: %d times", ex.i);
    }
}

static void ut_work_queue_test(void) {
    ut_work_queue_test_1();
    ut_work_queue_test_2();
    ut_work_queue_test_3();
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_traceln("done"); }
}

static int32_t ut_test_do_work_called;

static void ut_test_do_work(ut_work_t* ut_unused(w)) {
    ut_test_do_work_called++;
}

static void ut_worker_test(void) {
//  uncomment one of the following lines to see the output
//  ut_debug.verbosity.level = ut_debug.verbosity.info;
//  ut_debug.verbosity.level = ut_debug.verbosity.verbose;
    ut_work_queue_test(); // first test ut_work_queue
    ut_worker_t worker = { 0 };
    ut_worker.start(&worker);
    ut_work_t asap = {
        .when = 0, // A.S.A.P.
        .done = ut_event.create(),
        .work = ut_test_do_work
    };
    ut_work_t later = {
        .when = ut_clock.seconds() + 0.010, // 10ms
        .done = ut_event.create(),
        .work = ut_test_do_work
    };
    ut_worker.post(&worker, &asap);
    ut_worker.post(&worker, &later);
    // because `asap` and `later` are local variables
    // code needs to wait for them to be processed inside
    // this function before they goes out of scope
    ut_event.wait(asap.done); // await(asap)
    ut_event.dispose(asap.done); // responsibility of the caller
    // wait for later:
    ut_event.wait(later.done); // await(later)
    ut_event.dispose(later.done); // responsibility of the caller
    // quit the worker thread:
    ut_fatal_if_error(ut_worker.join(&worker, -1.0));
    // does worker respect .when dispatch time?
    swear(ut_clock.seconds() >= later.when);
}

#else

static void ut_work_queue_test(void) {}
static void ut_worker_test(void) {}

#endif
