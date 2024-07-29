#pragma once
#include "rt/rt.h"

rt_begin_c

// Minimalistic "react"-like work_queue or work items and
// a thread based workers. See rt_worker_test() for usage.

typedef struct rt_event_s*     rt_event_t;
typedef struct rt_work_s       rt_work_t;
typedef struct rt_work_queue_s rt_work_queue_t;

typedef struct rt_work_s {
    rt_work_queue_t* queue; // queue where the call is or was last scheduled
    fp64_t when;       // proc() call will be made after or at this time
    void (*work)(rt_work_t* c);
    void*  data;       // extra data that will be passed to proc() call
    rt_event_t  done;  // if not null signalled after calling proc() or canceling
    rt_work_t*  next;  // next element in the queue (implementation detail)
    bool    canceled;  // set to true inside .cancel() call
} rt_work_t;

typedef struct rt_work_queue_s {
    rt_work_t* head;
    int64_t    lock; // spinlock
    rt_event_t changed; // if not null will be signaled when head changes
} rt_work_queue_t;

typedef struct rt_work_queue_if {
    void (*post)(rt_work_t* c);
    bool (*get)(rt_work_queue_t*, rt_work_t* *c);
    void (*call)(rt_work_t* c);
    void (*dispatch)(rt_work_queue_t* q); // all ready messages
    void (*cancel)(rt_work_t* c);
    void (*flush)(rt_work_queue_t* q); // cancel all requests in the queue
} rt_work_queue_if;

extern rt_work_queue_if  rt_work_queue;

typedef struct rt_worker_s {
    rt_work_queue_t queue;
    rt_thread_t     thread;
    rt_event_t      wake;
    volatile bool   quit;
} rt_worker_t;

typedef struct rt_worker_if {
    void    (*start)(rt_worker_t* tq);
    void    (*post)(rt_worker_t* tq, rt_work_t* w);
    errno_t (*join)(rt_worker_t* tq, fp64_t timeout);
    void    (*test)(void);
} rt_worker_if;

extern rt_worker_if rt_worker;

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

rt_end_c

/*
    Usage examples:

    // The dispatch_until() is just for testing purposes.
    // Usually rt_work_queue.dispatch(q) will be called inside each
    // iteration of message loop of a dispatch [UI] thread.

    static void dispatch_until(rt_work_queue_t* q, int32_t* i, const int32_t n);

    // simple way of passing a single pointer to call_later

    static void every_100ms(rt_work_t* w) {
        int32_t* i = (int32_t*)w->data;
        rt_println("i: %d", *i);
        (*i)++;
        w->when = rt_clock.seconds() + 0.100;
        rt_work_queue.post(w);
    }

    static void example_1(void) {
        rt_work_queue_t queue = {0};
        // if a single pointer will suffice
        int32_t i = 0;
        rt_work_t work = {
            .queue = &queue,
            .when  = rt_clock.seconds() + 0.100,
            .work  = every_100ms,
            .data  = &i
        };
        rt_work_queue.post(&work);
        dispatch_until(&queue, &i, 4);
    }

    // extending rt_work_t with extra data:

    typedef struct rt_work_ex_s {
        union {
            rt_work_t base;
            struct rt_work_s;
        };
        struct { int32_t a; int32_t b; } s;
        int32_t i;
    } rt_work_ex_t;

    static void every_200ms(rt_work_t* w) {
        rt_work_ex_t* ex = (rt_work_ex_t*)w;
        rt_println("ex { .i: %d, .s.a: %d .s.b: %d}", ex->i, ex->s.a, ex->s.b);
        ex->i++;
        const int32_t swap = ex->s.a; ex->s.a = ex->s.b; ex->s.b = swap;
        w->when = rt_clock.seconds() + 0.200;
        rt_work_queue.post(w);
    }

    static void example_2(void) {
        rt_work_queue_t queue = {0};
        rt_work_ex_t work = {
            .queue = &queue,
            .when  = rt_clock.seconds() + 0.200,
            .work  = every_200ms,
            .data  = null,
            .s = { .a = 1, .b = 2 },
            .i = 0
        };
        rt_work_queue.post(&work.base);
        dispatch_until(&queue, &work.i, 4);
    }

    static void dispatch_until(rt_work_queue_t* q, int32_t* i, const int32_t n) {
        while (q->head != null && *i < n) {
            rt_thread.sleep_for(0.0001); // 100 microseconds
            rt_work_queue.dispatch(q);
        }
        rt_work_queue.flush(q);
    }

    // worker:

    static void do_work(rt_work_t* w) {
        // TODO: something useful
    }

    static void worker_test(void) {
        rt_worker_t worker = { 0 };
        rt_worker.start(&worker);
        rt_work_t work = {
            .when  = rt_clock.seconds() + 0.010, // 10ms
            .done  = rt_event.create(),
            .work  = do_work
        };
        rt_worker.post(&worker, &work);
        rt_event.wait(work.done);    // await(work)
        rt_event.dispose(work.done); // responsibility of the caller
        rt_fatal_if_error(rt_worker.join(&worker, -1.0));
    }

    // Hint:
    // To monitor timing turn on MSVC Output / Show Timestamp (clock button)

*/
