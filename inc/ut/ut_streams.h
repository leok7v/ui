#pragma once
#include "ut/ut_std.h"

begin_c

typedef struct ut_stream_if ut_stream_if;

typedef struct ut_stream_if {
    errno_t (*read)(ut_stream_if* s, void* data, int64_t bytes,
                    int64_t *transferred);
    errno_t (*write)(ut_stream_if* s, const void* data, int64_t bytes,
                     int64_t *transferred);
    void    (*close)(ut_stream_if* s); // optional
} ut_stream_if;

typedef struct ut_stream_memory_if {
    ut_stream_if   stream;
    const void* data_read;
    int64_t     bytes_read;
    int64_t     pos_read;
    void*       data_write;
    int64_t     bytes_write;
    int64_t     pos_write;
} ut_stream_memory_if;

typedef struct streams_if {
    void (*read_only)(ut_stream_memory_if* s,  const void* data, int64_t bytes);
    void (*write_only)(ut_stream_memory_if* s, void* data, int64_t bytes);
    void (*read_write)(ut_stream_memory_if* s, const void* read, int64_t read_bytes,
                                               void* write, int64_t write_bytes);
    void (*test)(void);
} ut_streams_if;

extern ut_streams_if ut_streams;

end_c
