#include "ut/ut.h"
#include "ut/win32.h"

static errno_t streams_memory_read(stream_if* stream, void* data, int64_t bytes,
        int64_t *transferred) {
    swear(bytes > 0);
    stream_memory_if* s = (stream_memory_if*)stream;
    swear(0 <= s->pos_read && s->pos_read <= s->bytes_read,
          "bytes: %lld stream .pos: %lld .bytes: %lld",
          bytes, s->pos_read, s->bytes_read);
    int64_t transfer = min(bytes, s->bytes_read - s->pos_read);
    memcpy(data, (uint8_t*)s->data_read + s->pos_read, transfer);
    s->pos_read += transfer;
    if (transferred != null) { *transferred = transfer; }
    return 0;
}

static errno_t streams_memory_write(stream_if* stream, const void* data, int64_t bytes,
        int64_t *transferred) {
    swear(bytes > 0);
    stream_memory_if* s = (stream_memory_if*)stream;
    swear(0 <= s->pos_write && s->pos_write <= s->bytes_write,
          "bytes: %lld stream .pos: %lld .bytes: %lld",
          bytes, s->pos_write, s->bytes_write);
    bool overflow = s->bytes_write - s->pos_write <= 0;
    int64_t transfer = min(bytes, s->bytes_write - s->pos_write);
    memcpy((uint8_t*)s->data_write + s->pos_write, data, transfer);
    s->pos_write += transfer;
    if (transferred != null) { *transferred = transfer; }
    return overflow ? ERROR_INSUFFICIENT_BUFFER : 0;
}

static void streams_read_only(stream_memory_if* s,
        const void* data, int64_t bytes) {
    s->stream.read = streams_memory_read;
    s->stream.write = null;
    s->data_read = data;
    s->bytes_read = bytes;
    s->pos_read = 0;
    s->data_write = null;
    s->bytes_write = 0;
    s->pos_write = 0;
}

static void streams_write_only(stream_memory_if* s,
        void* data, int64_t bytes) {
    s->stream.read = null;
    s->stream.write = streams_memory_write;
    s->data_read = null;
    s->bytes_read = 0;
    s->pos_read = 0;
    s->data_write = data;
    s->bytes_write = bytes;
    s->pos_write = 0;
}

static void streams_read_write(stream_memory_if* s,
        const void* read, int64_t read_bytes,
        void* write, int64_t write_bytes) {
    s->stream.read = streams_memory_read;
    s->stream.write = streams_memory_write;
    s->data_read = read;
    s->bytes_read = read_bytes;
    s->pos_read = 0;
    s->pos_read = 0;
    s->data_write = write;
    s->bytes_write = write_bytes;
    s->pos_write = 0;
}

#ifdef RUNTIME_TESTS

static void streams_test(void) {
    {   // read test
        uint8_t memory[256];
        for (int32_t i = 0; i < countof(memory); i++) { memory[i] = (uint8_t)i; }
        for (int32_t i = 1; i < countof(memory) - 1; i++) {
            stream_memory_if ms; // memory stream
            streams.read_only(&ms, memory, sizeof(memory));
            uint8_t data[256];
            for (int32_t j = 0; j < countof(data); j++) { data[j] = 0xFF; }
            int64_t transferred = 0;
            errno_t r = ms.stream.read(&ms.stream, data, i, &transferred);
            swear(r == 0 && transferred == i);
            for (int32_t j = 0; j < i; j++) { swear(data[j] == memory[j]); }
            for (int32_t j = i; j < countof(data); j++) { swear(data[j] == 0xFF); }
        }
    }
    {   // write test
        // TODO: implement
    }
    {   // read/write test
        // TODO: implement
    }
    if (debug.verbosity.level > debug.verbosity.quiet) { traceln("done"); }
}

#else

static void streams_test(void) { }

#endif

streams_if streams = {
    .read_only  = streams_read_only,
    .write_only = streams_write_only,
    .read_write = streams_read_write,
    .test = streams_test
};
