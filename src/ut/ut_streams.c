#include "ut/ut.h"
#include "ut/ut_win32.h"

static errno_t rt_streams_memory_read(rt_stream_if* stream, void* data, int64_t bytes,
        int64_t *transferred) {
    rt_swear(bytes > 0);
    rt_stream_memory_if* s = (rt_stream_memory_if*)stream;
    rt_swear(0 <= s->pos_read && s->pos_read <= s->bytes_read,
          "bytes: %lld stream .pos: %lld .bytes: %lld",
          bytes, s->pos_read, s->bytes_read);
    int64_t transfer = rt_min(bytes, s->bytes_read - s->pos_read);
    memcpy(data, (const uint8_t*)s->data_read + s->pos_read, (size_t)transfer);
    s->pos_read += transfer;
    if (transferred != null) { *transferred = transfer; }
    return 0;
}

static errno_t rt_streams_memory_write(rt_stream_if* stream, const void* data, int64_t bytes,
        int64_t *transferred) {
    rt_swear(bytes > 0);
    rt_stream_memory_if* s = (rt_stream_memory_if*)stream;
    rt_swear(0 <= s->pos_write && s->pos_write <= s->bytes_write,
          "bytes: %lld stream .pos: %lld .bytes: %lld",
          bytes, s->pos_write, s->bytes_write);
    bool overflow = s->bytes_write - s->pos_write <= 0;
    int64_t transfer = rt_min(bytes, s->bytes_write - s->pos_write);
    memcpy((uint8_t*)s->data_write + s->pos_write, data, (size_t)transfer);
    s->pos_write += transfer;
    if (transferred != null) { *transferred = transfer; }
    return overflow ? ERROR_INSUFFICIENT_BUFFER : 0;
}

static void rt_streams_read_only(rt_stream_memory_if* s,
        const void* data, int64_t bytes) {
    s->stream.read = rt_streams_memory_read;
    s->stream.write = null;
    s->data_read = data;
    s->bytes_read = bytes;
    s->pos_read = 0;
    s->data_write = null;
    s->bytes_write = 0;
    s->pos_write = 0;
}

static void rt_streams_write_only(rt_stream_memory_if* s,
        void* data, int64_t bytes) {
    s->stream.read = null;
    s->stream.write = rt_streams_memory_write;
    s->data_read = null;
    s->bytes_read = 0;
    s->pos_read = 0;
    s->data_write = data;
    s->bytes_write = bytes;
    s->pos_write = 0;
}

static void rt_streams_read_write(rt_stream_memory_if* s,
        const void* read, int64_t read_bytes,
        void* write, int64_t write_bytes) {
    s->stream.read = rt_streams_memory_read;
    s->stream.write = rt_streams_memory_write;
    s->data_read = read;
    s->bytes_read = read_bytes;
    s->pos_read = 0;
    s->pos_read = 0;
    s->data_write = write;
    s->bytes_write = write_bytes;
    s->pos_write = 0;
}

#ifdef UT_TESTS

static void rt_streams_test(void) {
    {   // read test
        uint8_t memory[256];
        for (int32_t i = 0; i < rt_countof(memory); i++) { memory[i] = (uint8_t)i; }
        for (int32_t i = 1; i < rt_countof(memory) - 1; i++) {
            rt_stream_memory_if ms; // memory stream
            rt_streams.read_only(&ms, memory, sizeof(memory));
            uint8_t data[256];
            for (int32_t j = 0; j < rt_countof(data); j++) { data[j] = 0xFF; }
            int64_t transferred = 0;
            errno_t r = ms.stream.read(&ms.stream, data, i, &transferred);
            rt_swear(r == 0 && transferred == i);
            for (int32_t j = 0; j < i; j++) { rt_swear(data[j] == memory[j]); }
            for (int32_t j = i; j < rt_countof(data); j++) { rt_swear(data[j] == 0xFF); }
        }
    }
    {   // write test
        // TODO: implement
    }
    {   // read/write test
        // TODO: implement
    }
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { rt_println("done"); }
}

#else

static void rt_streams_test(void) { }

#endif

rt_streams_if rt_streams = {
    .read_only  = rt_streams_read_only,
    .write_only = rt_streams_write_only,
    .read_write = rt_streams_read_write,
    .test       = rt_streams_test
};
