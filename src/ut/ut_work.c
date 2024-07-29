#include "ut/ut.h"

static void rt_work_queue_no_duplicates(rt_work_t* w) {
    rt_work_t* e = w->queue->head;
    bool found = false;
    while (e != null && !found) {
        found = e == w;
        if (!found) { e = e->next; }
    }
    rt_swear(!found);
}

static void rt_work_queue_post(rt_work_t* w) {
    rt_assert(w->queue != null && w != null && w->when >= 0.0);
    rt_work_queue_t* q = w->queue;
    rt_atomics.spinlock_acquire(&q->lock);
    rt_work_queue_no_duplicates(w); // under lock
    //  Enqueue in time sorted order least ->time first to save
    //  time searching in fetching from queue which is more frequent.
    rt_work_t* p = null;
    rt_work_t* e = q->head;
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
    rt_atomics.spinlock_release(&q->lock);
    if (head && q->changed != null) { rt_event.set(q->changed); }
}

static void rt_work_queue_cancel(rt_work_t* w) {
    rt_swear(!w->canceled && w->queue != null && w->queue->head != null);
    rt_work_queue_t* q = w->queue;
    rt_atomics.spinlock_acquire(&q->lock);
    rt_work_t* p = null;
    rt_work_t* e = q->head;
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
    rt_atomics.spinlock_release(&q->lock);
    rt_swear(w->canceled);
    if (w->done != null) { rt_event.set(w->done); }
    if (changed && q->changed != null) { rt_event.set(q->changed); }
}

static void rt_work_queue_flush(rt_work_queue_t* q) {
    while (q->head != null) { rt_work_queue.cancel(q->head); }
}

static bool rt_work_queue_get(rt_work_queue_t* q, rt_work_t* *r) {
    rt_work_t* w = null;
    rt_atomics.spinlock_acquire(&q->lock);
    bool changed = q->head != null && q->head->when <= rt_clock.seconds();
    if (changed) {
        w = q->head;
        q->head = w->next;
        w->next = null;
    }
    rt_atomics.spinlock_release(&q->lock);
    *r = w;
    if (changed && q->changed != null) { rt_event.set(q->changed); }
    return w != null;
}

static void rt_work_queue_call(rt_work_t* w) {
    if (w->work != null) { w->work(w); }
    if (w->done != null) { rt_event.set(w->done); }
}

static void rt_work_queue_dispatch(rt_work_queue_t* q) {
    rt_work_t* w = null;
    while (rt_work_queue.get(q, &w)) { rt_work_queue.call(w); }
}

rt_work_queue_if rt_work_queue = {
    .post     = rt_work_queue_post,
    .get      = rt_work_queue_get,
    .call     = rt_work_queue_call,
    .dispatch = rt_work_queue_dispatch,
    .cancel   = rt_work_queue_cancel,
    .flush    = rt_work_queue_flush
};

static void rt_worker_thread(void* p) {
    rt_thread.name("worker");
    rt_worker_t* worker = (rt_worker_t*)p;
    rt_work_queue_t* q = &worker->queue;
    while (!worker->quit) {
        rt_work_queue.dispatch(q);
        fp64_t timeout = -1.0; // forever
        rt_atomics.spinlock_acquire(&q->lock);
        if (q->head != null) {
            timeout = rt_max(0, q->head->when - rt_clock.seconds());
        }
        rt_atomics.spinlock_release(&q->lock);
        // if another item is inserted into head after unlocking
        // the `wake` event guaranteed to be signalled
        if (!worker->quit && timeout != 0) {
            rt_event.wait_or_timeout(worker->wake, timeout);
        }
    }
    rt_work_queue.dispatch(q);
}

static void rt_worker_start(rt_worker_t* worker) {
    rt_assert(worker->wake == null && !worker->quit);
    worker->wake  = rt_event.create();
    worker->queue = (rt_work_queue_t){
        .head = null, .lock = 0, .changed = worker->wake
    };
    worker->thread = rt_thread.start(rt_worker_thread, worker);
}

static errno_t rt_worker_join(rt_worker_t* worker, fp64_t to) {
    worker->quit = true;
    rt_event.set(worker->wake);
    errno_t r = rt_thread.join(worker->thread, to);
    if (r == 0) {
        rt_event.dispose(worker->wake);
        worker->wake = null;
        worker->thread = null;
        worker->quit = false;
        rt_swear(worker->queue.head == null);
    }
    return r;
}

static void rt_worker_post(rt_worker_t* worker, rt_work_t* w) {
    rt_assert(!worker->quit && worker->wake != null && worker->thread != null);
    w->queue = &worker->queue;
    rt_work_queue.post(w);
}

static void rt_worker_test(void);

rt_worker_if rt_worker = {
    .start = rt_worker_start,
    .post  = rt_worker_post,
    .join  = rt_worker_join,
    .test  = rt_worker_test
};

#ifdef UT_TESTS

// tests:

// keep in mind that rt_println() may be blocking and is a subject
// of "astronomical" wait state times in order of dozens of ms.

static int32_t ut_test_called;

static void ut_never_called(rt_work_t* rt_unused(w)) {
    ut_test_called++;
}

static void rt_work_queue_test_1(void) {
    ut_test_called = 0;
    // testing insertion time ordering of two events into queue
    const fp64_t now = rt_clock.seconds();
    rt_work_queue_t q = {0};
    rt_work_t c1 = {
        .queue = &q,
        .work = ut_never_called,
        .when = now + 1.0
    };
    rt_work_t c2 = {
        .queue = &q,
        .work = ut_never_called,
        .when = now + 0.5
    };
    rt_work_queue.post(&c1);
    rt_swear(q.head == &c1 && q.head->next == null);
    rt_work_queue.post(&c2);
    rt_swear(q.head == &c2 && q.head->next == &c1);
    rt_work_queue.flush(&q);
    // test that canceled events are not dispatched
    rt_swear(ut_test_called == 0 && c1.canceled && c2.canceled && q.head == null);
    c1.canceled = false;
    c2.canceled = false;
    // test the rt_work_queue.cancel() function
    rt_work_queue.post(&c1);
    rt_work_queue.post(&c2);
    rt_swear(q.head == &c2 && q.head->next == &c1);
    rt_work_queue.cancel(&c2);
    rt_swear(c2.canceled && q.head == &c1 && q.head->next == null);
    c2.canceled = false;
    rt_work_queue.post(&c2);
    rt_work_queue.cancel(&c1);
    rt_swear(c1.canceled && q.head == &c2 && q.head->next == null);
    rt_work_queue.flush(&q);
    rt_swear(ut_test_called == 0 && c1.canceled && c2.canceled && q.head == null);
}

// simple way of passing a single pointer to call_later

static fp64_t ut_test_work_start; // makes timing debug traces easier to read

static void ut_every_millisecond(rt_work_t* w) {
    int32_t* i = (int32_t*)w->data;
    fp64_t now = rt_clock.seconds();
    if (rt_debug.verbosity.level > rt_debug.verbosity.info) {
        const fp64_t since_start = now - ut_test_work_start;
        const fp64_t dt = w->when - ut_test_work_start;
        rt_println("%d now: %.6f time: %.6f", *i, since_start, dt);
    }
    (*i)++;
    // read rt_clock.seconds() again because rt_println() above could block
    w->when = rt_clock.seconds() + 0.001;
    rt_work_queue.post(w);
}

static void rt_work_queue_test_2(void) {
    rt_thread.realtime();
    ut_test_work_start = rt_clock.seconds();
    rt_work_queue_t q = {0};
    // if a single pointer will suffice
    int32_t i = 0;
    rt_work_t c = {
        .queue = &q,
        .work = ut_every_millisecond,
        .when = ut_test_work_start + 0.001,
        .data = &i
    };
    rt_work_queue.post(&c);
    while (q.head != null && i < 8) {
        rt_thread.sleep_for(0.0001); // 100 microseconds
        rt_work_queue.dispatch(&q);
    }
    rt_work_queue.flush(&q);
    rt_swear(q.head == null);
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) {
        rt_println("called: %d times", i);
    }
}

// extending rt_work_t with extra data:

typedef struct rt_work_ex_s {
    // nameless union opens up base fields into rt_work_ex_t
    // it is not necessary at all
    union {
        rt_work_t base;
        struct rt_work_s;
    };
    struct { int32_t a; int32_t b; } s;
    int32_t i;
} rt_work_ex_t;

static void ut_every_other_millisecond(rt_work_t* w) {
    rt_work_ex_t* ex = (rt_work_ex_t*)w;
    fp64_t now = rt_clock.seconds();
    if (rt_debug.verbosity.level > rt_debug.verbosity.info) {
        const fp64_t since_start = now - ut_test_work_start;
        const fp64_t dt  = w->when - ut_test_work_start;
        rt_println(".i: %d .extra: {.a: %d .b: %d} now: %.6f time: %.6f",
                ex->i, ex->s.a, ex->s.b, since_start, dt);
    }
    ex->i++;
    const int32_t swap = ex->s.a; ex->s.a = ex->s.b; ex->s.b = swap;
    // read rt_clock.seconds() again because rt_println() above could block
    w->when = rt_clock.seconds() + 0.002;
    rt_work_queue.post(w);
}

static void rt_work_queue_test_3(void) {
    rt_thread.realtime();
    rt_static_assertion(offsetof(rt_work_ex_t, base) == 0);
    const fp64_t now = rt_clock.seconds();
    rt_work_queue_t q = {0};
    rt_work_ex_t ex = {
        .queue = &q,
        .work = ut_every_other_millisecond,
        .when = now + 0.002,
        .s = { .a = 1, .b = 2 },
        .i = 0
    };
    rt_work_queue.post(&ex.base);
    while (q.head != null && ex.i < 8) {
        rt_thread.sleep_for(0.0001); // 100 microseconds
        rt_work_queue.dispatch(&q);
    }
    rt_work_queue.flush(&q);
    rt_swear(q.head == null);
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) {
        rt_println("called: %d times", ex.i);
    }
}

static void rt_work_queue_test(void) {
    rt_work_queue_test_1();
    rt_work_queue_test_2();
    rt_work_queue_test_3();
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

static int32_t ut_test_do_work_called;

static void ut_test_do_work(rt_work_t* rt_unused(w)) {
    ut_test_do_work_called++;
}

static void rt_worker_test(void) {
//  uncomment one of the following lines to see the output
//  rt_debug.verbosity.level = rt_debug.verbosity.info;
//  rt_debug.verbosity.level = rt_debug.verbosity.verbose;
    rt_work_queue_test(); // first test rt_work_queue
    rt_worker_t worker = { 0 };
    rt_worker.start(&worker);
    rt_work_t asap = {
        .when = 0, // A.S.A.P.
        .done = rt_event.create(),
        .work = ut_test_do_work
    };
    rt_work_t later = {
        .when = rt_clock.seconds() + 0.010, // 10ms
        .done = rt_event.create(),
        .work = ut_test_do_work
    };
    rt_worker.post(&worker, &asap);
    rt_worker.post(&worker, &later);
    // because `asap` and `later` are local variables
    // code needs to wait for them to be processed inside
    // this function before they goes out of scope
    rt_event.wait(asap.done); // await(asap)
    rt_event.dispose(asap.done); // responsibility of the caller
    // wait for later:
    rt_event.wait(later.done); // await(later)
    rt_event.dispose(later.done); // responsibility of the caller
    // quit the worker thread:
    rt_fatal_if_error(rt_worker.join(&worker, -1.0));
    // does worker respect .when dispatch time?
    rt_swear(rt_clock.seconds() >= later.when);
}

#else

static void rt_work_queue_test(void) {}
static void rt_worker_test(void) {}

#endif
