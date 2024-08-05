/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct ui_midi_s ui_midi_t;

typedef struct ui_midi_s {
    uint8_t data[16 * 8]; // opaque implementation data
    // must return 0 if successful or error otherwise:
    int64_t (*notify)(ui_midi_t* midi, int64_t flags);
} ui_midi_t;

typedef struct {
    // flags bitset:
    int32_t const success; // when the clip is done playing
    int32_t const failure; // on error playing media
    int32_t const aborted; // on stop() call
    int32_t const superseded;
    // midi has it's own section of legacy error messages
    void    (*error)(errno_t r, char* s, int32_t count);
    errno_t (*open)(ui_midi_t* midi, const char* filename);
    errno_t (*play)(ui_midi_t* midi);
    errno_t (*rewind)(ui_midi_t* midi);
    errno_t (*stop)(ui_midi_t* midi);
    errno_t (*get_volume)(ui_midi_t* midi, fp64_t *volume);
    errno_t (*set_volume)(ui_midi_t* midi, fp64_t  volume);
    bool    (*is_open)(ui_midi_t* midi);
    bool    (*is_playing)(ui_midi_t* midi);
    void    (*close)(ui_midi_t* midi);
} ui_midi_if;

extern ui_midi_if ui_midi;


/*
    success:
    "The conditions initiating the callback function have been met."
    I guess meaning media is done playing...

    failure:
    "A device error occurred while the device was executing the command."

    aborted:
    "The device received a command that prevented the current
    conditions for initiating the callback function from
    being met. If a new command interrupts the current command
    and it also requests notification, the device sends this
    message only and not `superseded`".
    I guess meaning media is stopped playing...

    superseded:
    "The device received another command with the "notify" flag set
     and the current conditions for initiating the callback function
     have been superseded."
*/

#ifdef __cplusplus
}
#endif



