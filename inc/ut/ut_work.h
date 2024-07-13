#pragma once
#include "ut/ut.h"

begin_c

// Minimalistic "react"-like work_queue or work items and
// a thread based workers. See ut_worker_test() for usage.

typedef struct ut_event_s*     ut_event_t;
typedef struct ut_work_s       ut_work_t;
typedef struct ut_work_queue_s ut_work_queue_t;

typedef struct ut_work_s {
    ut_work_queue_t* queue; // queue where the call is or was last scheduled
    fp64_t when;       // proc() call will be made after or at this time
    void (*ui_fuzzing_work)(ut_work_t* c);
    void*  data;       // extra data that will be passed to proc() call
    ut_event_t  done;  // if not null signalled after calling proc() or canceling
    ut_work_t*  next;  // next element in the queue (implementation detail)
    bool    canceled;  // set to true inside .cancel() call
} ut_work_t;

typedef struct ut_work_queue_s {
    ut_work_t* head;
    int64_t    lock; // spinlock
    ut_event_t changed; // if not null will be signaled when head changes
} ut_work_queue_t;

typedef struct ut_work_queue_if {
    void (*post)(ut_work_t* c);
    bool (*get)(ut_work_queue_t*, ut_work_t* *c);
    void (*call)(ut_work_t* c);
    void (*dispatch)(ut_work_queue_t* q); // all ready messages
    void (*cancel)(ut_work_t* c);
    void (*flush)(ut_work_queue_t* q); // cancel all requests in the queue
} ut_work_queue_if;

extern ut_work_queue_if  ut_work_queue;

typedef struct ut_worker_s {
    ut_work_queue_t queue;
    ut_thread_t     thread;
    ut_event_t      wake;
    volatile bool   quit;
} ut_worker_t;

typedef struct ut_worker_if {
    void    (*start)(ut_worker_t* tq);
    void    (*post)(ut_worker_t* tq, ut_work_t* w);
    errno_t (*join)(ut_worker_t* tq, fp64_t timeout);
    void    (*test)(void);
} ut_worker_if;

extern ut_worker_if ut_worker;

// worker thread waits for a queue's `wake` event with the timeout
// infinity or if queue is not empty delta time till the head
// item of the queue.
//
// Upon post() call the `wake` event is set and the worker thread
// wakes up and dispatches all the items with .when less then now
// calling function work() if it is not null and optionally signaling
// .done event if it is not null.
//
// When all ready items in the queue are processed worker thread locks
// the queue and if the head is present calculates next timeout based
// on .when time of the head or sets timeout to infinity if the queue
// is empty.
//
// Function .join() sets .quit to true signals .wake event and attempt
// to join the worker .thread with specified timeout.
// It is the responsibility of the caller to ensure that no other
// work is posted after calling .join() because it will be lost.

end_c

/*
    Usage examples:

    // The dispatch_until() is just for testing purposes.
    // Usually ut_work_queue.dispatch(q) will be called inside each
    // iteration of message loop of a dispatch [UI] thread.

    static void dispatch_until(ut_work_queue_t* q, int32_t* i, const int32_t n);

    // simple way of passing a single pointer to call_later

    static void every_100ms(ut_work_t* w) {
        int32_t* i = (int32_t*)w->data;
        ut_traceln("i: %d", *i);
        (*i)++;
        w->when = ut_clock.seconds() + 0.100;
        ut_work_queue.post(w);
    }

    static void example_1(void) {
        ut_work_queue_t queue = {0};
        // if a single pointer will suffice
        int32_t i = 0;
        ut_work_t work = {
            .queue = &queue,
            .when  = ut_clock.seconds() + 0.100,
            .work  = every_100ms,
            .data  = &i
        };
        ut_work_queue.post(&work);
        dispatch_until(&queue, &i, 4);
    }

    // extending ut_work_t with extra data:

    typedef struct ut_work_ex_s {
        union {
            ut_work_t base;
            struct ut_work_s;
        };
        struct { int32_t a; int32_t b; } s;
        int32_t i;
    } ut_work_ex_t;

    static void every_200ms(ut_work_t* w) {
        ut_work_ex_t* ex = (ut_work_ex_t*)w;
        ut_traceln("ex { .i: %d, .s.a: %d .s.b: %d}", ex->i, ex->s.a, ex->s.b);
        ex->i++;
        const int32_t swap = ex->s.a; ex->s.a = ex->s.b; ex->s.b = swap;
        w->when = ut_clock.seconds() + 0.200;
        ut_work_queue.post(w);
    }

    static void example_2(void) {
        ut_work_queue_t queue = {0};
        ut_work_ex_t work = {
            .queue = &queue,
            .when  = ut_clock.seconds() + 0.200,
            .work  = every_200ms,
            .data  = null,
            .s = { .a = 1, .b = 2 },
            .i = 0
        };
        ut_work_queue.post(&work.base);
        dispatch_until(&queue, &work.i, 4);
    }

    static void dispatch_until(ut_work_queue_t* q, int32_t* i, const int32_t n) {
        while (q->head != null && *i < n) {
            ut_thread.sleep_for(0.0001); // 100 microseconds
            ut_work_queue.dispatch(q);
        }
        ut_work_queue.flush(q);
    }

    // worker:

    static void do_work(ut_work_t* w) {
        // TODO: something useful
    }

    static void worker_test(void) {
        ut_worker_t worker = { 0 };
        ut_worker.start(&worker);
        ut_work_t work = {
            .when  = ut_clock.seconds() + 0.010, // 10ms
            .done  = ut_event.create(),
            .work  = do_work
        };
        ut_worker.post(&worker, &work);
        ut_event.wait(work.done);    // await(work)
        ut_event.dispose(work.done); // responsibility of the caller
        ut_fatal_if_error(ut_worker.join(&worker, -1.0));
    }

    // Hint:
    // To monitor timing turn on MSVC Output / Show Timestamp (clock button)

*/
