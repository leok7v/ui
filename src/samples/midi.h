/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include <stdint.h>

#ifdef __cplusplus
    extern "C" {
#endif


typedef struct midi_s {
    void* data[16]; // opaque
    int64_t device_id; // lp of notify message
} midi_t;

typedef struct {
    int32_t const notify;     // message
    // wp bitset, lp device_id
    int32_t const successful;
    int32_t const superseded;
    int32_t const aborted;
    int32_t const failure;
    int32_t const success;
    errno_t (*open)(midi_t* midi_t, void* window, const char* filename);
    errno_t (*play)(midi_t* midi_t);
    errno_t (*stop)(midi_t* midi_t);
    void    (*close)(midi_t* midi_t);
} midi_if;

extern midi_if midi;

#ifdef __cplusplus
}
#endif



