#pragma once
#include "rt/rt_std.h"

rt_begin_c

typedef struct rt_stream_if rt_stream_if;

typedef struct rt_stream_if {
    errno_t (*read)(rt_stream_if* s, void* data, int64_t bytes,
                    int64_t *transferred);
    errno_t (*write)(rt_stream_if* s, const void* data, int64_t bytes,
                     int64_t *transferred);
    void    (*close)(rt_stream_if* s); // optional
} rt_stream_if;

typedef struct {
    rt_stream_if   stream;
    const void* data_read;
    int64_t     bytes_read;
    int64_t     pos_read;
    void*       data_write;
    int64_t     bytes_write;
    int64_t     pos_write;
} rt_stream_memory_if;

typedef struct {
    void (*read_only)(rt_stream_memory_if* s,  const void* data, int64_t bytes);
    void (*write_only)(rt_stream_memory_if* s, void* data, int64_t bytes);
    void (*read_write)(rt_stream_memory_if* s, const void* read, int64_t read_bytes,
                                               void* write, int64_t write_bytes);
    void (*test)(void);
} rt_streams_if;

extern rt_streams_if rt_streams;

rt_end_c
