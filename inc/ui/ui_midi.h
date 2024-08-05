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
    int32_t const aborted; // on stop() call
    int32_t const failure; // on error playing media
    int32_t const success; // when the clip is done playing
    errno_t (*open)(ui_midi_t* midi, const char* filename);
    errno_t (*play)(ui_midi_t* midi);
    errno_t (*pause)(ui_midi_t* midi);
    errno_t (*resume)(ui_midi_t* midi);  // does not work for midi seq
    errno_t (*rewind)(ui_midi_t* midi);
    errno_t (*stop)(ui_midi_t* midi);
    errno_t (*get_volume)(ui_midi_t* midi, fp64_t *volume);
    errno_t (*set_volume)(ui_midi_t* midi, fp64_t  volume);
    bool    (*is_open)(ui_midi_t* midi);
    bool    (*is_playing)(ui_midi_t* midi);
    bool    (*is_paused)(ui_midi_t* midi);
    void    (*close)(ui_midi_t* midi);
} ui_midi_if;

extern ui_midi_if ui_midi;


/*
    successful:
    "The conditions initiating the callback function have been met."
    I guess meaning media is done playing...

    aborted:
    "The device received a command that prevented the current
    conditions for initiating the callback function from
    being met. If a new command interrupts the current command
    and it also requests notification, the device sends this
    message only and not `superseded`".
    I guess meaning media is stopped playing...

    failure:
    "A device error occurred while the device was executing the command."


*/

#ifdef __cplusplus
}
#endif



