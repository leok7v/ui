/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
    extern "C" {
#endif

struct ui_midi;

struct ui_midi {
    uint8_t data[16 * 8]; // opaque implementation data
    // must return 0 if successful or error otherwise:
    int64_t (*notify)(struct ui_midi* midi, int64_t flags);
};

struct ui_midi_if {
    // flags bitset:
    int32_t const success; // when the clip is done playing
    int32_t const failure; // on error playing media
    int32_t const aborted; // on stop() call
    int32_t const superseded;
    // midi has it's own section of legacy error messages
    void    (*error)(errno_t r, char* s, int32_t count);
    errno_t (*open)(struct ui_midi* midi, const char* filename);
    errno_t (*play)(struct ui_midi* midi);
    errno_t (*rewind)(struct ui_midi* midi);
    errno_t (*stop)(struct ui_midi* midi);
    errno_t (*get_volume)(struct ui_midi* midi, fp64_t *volume);
    errno_t (*set_volume)(struct ui_midi* midi, fp64_t  volume);
    bool    (*is_open)(struct ui_midi* midi);
    bool    (*is_playing)(struct ui_midi* midi);
    void    (*close)(struct ui_midi* midi);
};

extern struct ui_midi_if ui_midi;


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



