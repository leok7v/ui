#pragma once
#include "runtime/manifest.h"

begin_c

typedef struct stream_if stream_if;

typedef struct stream_if {
    errno_t (*read)(stream_if* s, void* data, int64_t bytes,
                    int64_t *transferred);
    errno_t (*write)(stream_if* s, const void* data, int64_t bytes,
                     int64_t *transferred);
} stream_if;

typedef struct stream_memory_if {
    stream_if   stream;
    const void* data_read;
    int64_t     bytes_read;
    int64_t     pos_read;
    void*       data_write;
    int64_t     bytes_write;
    int64_t     pos_write;
} stream_memory_if;

typedef struct streams_if {
    void (*read_only)(stream_memory_if* s,  const void* data, int64_t bytes);
    void (*write_only)(stream_memory_if* s, void* data, int64_t bytes);
    void (*read_write)(stream_memory_if* s, const void* read, int64_t read_bytes,
                                            void* write, int64_t write_bytes);
    void (*test)(void);
} streams_if;

extern streams_if streams;

end_c
