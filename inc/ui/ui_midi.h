/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct ui_midi_s ui_midi_t;

typedef struct ui_midi_s {
    void* data[16 + 8 * 4]; // opaque
    // must return 0 if successful or error otherwise:
    int64_t (*notify)(ui_midi_t* midi, int64_t flags);
} ui_midi_t;

typedef struct {
    // flags bitset:
    int32_t const successful;
    int32_t const superseded;
    int32_t const aborted;
    int32_t const failure;
    int32_t const success;
    errno_t (*open)(ui_midi_t* midi, const char* filename);
    errno_t (*play)(ui_midi_t* midi);
    errno_t (*stop)(ui_midi_t* midi);
    void    (*close)(ui_midi_t* midi);
} ui_midi_if;

extern ui_midi_if ui_midi;

#ifdef __cplusplus
}
#endif



