/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "posix.h"
#include "ui/ui_win32.h"
#include "ui/ui.h"
#include <mmsystem.h>

#pragma comment(lib, "winmm")

struct ui_midi_implementation {
    MCI_OPEN_PARMSA mop; // opaque
    struct ui_app_message_handler handler;
    char alias[32];
    int64_t device_id;
    uintptr_t window;
    bool playing;
};

posix_static_assertion(sizeof(struct ui_midi) >= sizeof(struct ui_midi_implementation) + sizeof(void*));
posix_static_assertion(MMSYSERR_NOERROR == 0);

static void ui_midi_error(errno_t r, char* text, int32_t count) {
    posix_fatal_win32err(mciGetErrorStringA(r, text, (UINT)count));
}

static void ui_midi_warn_if_error_(int r, const char* call, const char* func,
        int line) {
    if (r != 0) {
        static char error[256];
        ui_midi_error(r, error, posix_countof(error));
        posix_println("%s:%d %s", func, line, call);
        posix_println("%d - MCIERR_BASE: %d %s", r, r - MCIERR_BASE, error);
    }
}

#define ui_midi_warn_if_error(r) do {                  \
    ui_midi_warn_if_error_(r, #r, __func__, __LINE__); \
} while (0)

#define ui_midi_fatal_if_error(call) do {                                   \
    int _r_ = call; ui_midi_warn_if_error_(r, #call, __func__, __LINE__);   \
    posix_fatal_if_error(r);                                                   \
} while (0)

static bool ui_midi_message_callback(struct ui_app_message_handler* h, int32_t m,
                                     int64_t wp, int64_t lp, int64_t* rt) {
    if (m == MM_MCINOTIFY) {
        #ifdef UI_MIDI_DEBUG
            posix_println("device_id: %lld", lp);
            if (wp & MCI_NOTIFY_SUCCESSFUL) { posix_println("SUCCESSFUL"); }
            if (wp & MCI_NOTIFY_SUPERSEDED) { posix_println("SUPERSEDED"); }
            if (wp & MCI_NOTIFY_ABORTED)    { posix_println("ABORTED");    }
            if (wp & MCI_NOTIFY_FAILURE)    { posix_println("FAILURE");    }
        #endif
        struct ui_midi* midi = (struct ui_midi*)h->that;
        struct ui_midi_implementation* mi  = (struct ui_midi_implementation*)midi;
        if (mi->device_id == lp) {
            if (midi->notify != null) {
                *rt = midi->notify(midi, wp);
            } else {
                *rt = 0;
            }
            return true;
        }
    }
    return false;
}

static void ui_midi_remove_handler(struct ui_midi* m) {
    struct ui_midi_implementation* mi  = (struct ui_midi_implementation*)m;
    struct ui_app_message_handler* h = ui_app.handlers;
    if (h == &mi->handler) {
        ui_app.handlers = h->next;
    } else {
        while (h->next != null && h->next != &mi->handler) {
            h = h->next;
        }
        posix_swear(h->next == &mi->handler);
        if (h->next == &mi->handler) {
            h->next = h->next->next;
        }
    }
    mi->handler.callback = null;
    mi->handler.that = null;
    mi->handler.next = null;
}

static errno_t ui_midi_open(struct ui_midi* m, const char* filename) {
    posix_swear(posix_thread.id() == ui_app.tid);
    struct ui_midi_implementation* mi = (struct ui_midi_implementation*)m;
    mi->handler.that = mi;
    mi->handler.next = ui_app.handlers;
    ui_app.handlers = &mi->handler;
    mi->window = (uintptr_t)ui_app.window;
    mi->playing = false;
    mi->mop.dwCallback = mi->window;
    mi->mop.wDeviceID = (WORD)-1;
    mi->mop.lpstrDeviceType = (const char*)MCI_DEVTYPE_SEQUENCER;
    mi->mop.lpstrElementName = filename;
    mi->mop.lpstrAlias = mi->alias;
    posix_str_printf(mi->alias, "%p", m);
    const DWORD_PTR flags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
                            MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS;
    errno_t r = mciSendCommandA(0, MCI_OPEN, flags, (uintptr_t)&mi->mop);
    ui_midi_warn_if_error(r);
    posix_assert(mi->mop.wDeviceID != -1);
    mi->handler.callback = ui_midi_message_callback,
    mi->device_id = mi->mop.wDeviceID;
    if (r != 0) {
        ui_midi_remove_handler(m);
        memset(&mi->mop, 0x00, sizeof(mi->mop));
        mi->window = 0;
    }
    return r;
}

static errno_t ui_midi_play(struct ui_midi* m) {
    posix_swear(posix_thread.id() == ui_app.tid);
    struct ui_midi_implementation* mi = (struct ui_midi_implementation*)m;
    posix_swear(ui_midi.is_open(m));
    MCI_PLAY_PARMS  pp = { .dwCallback = (uintptr_t)mi->window };
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_PLAY, MCI_NOTIFY, (uintptr_t)&pp);
    ui_midi_warn_if_error(r);
    if (r == 0) {
        mi->playing = true;
    }
    return r;
}

static errno_t ui_midi_rewind(struct ui_midi* m) {
    posix_swear(posix_thread.id() == ui_app.tid);
    posix_swear(ui_midi.is_open(m));
    struct ui_midi_implementation* mi = (struct ui_midi_implementation*)m;
    MCI_SEEK_PARMS p = { .dwCallback = (uintptr_t)mi->window, .dwTo = 0 };
    const DWORD f = MCI_WAIT|MCI_SEEK_TO_START;
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_SEEK, f, (DWORD_PTR)&p);
    ui_midi_warn_if_error(r);
    return r;
}

static errno_t ui_midi_get_volume(struct ui_midi* m, fp64_t* volume) {
    posix_swear(posix_thread.id() == ui_app.tid);
    posix_swear(ui_midi.is_open(m) && ui_midi.is_playing(m));
    DWORD v = 0;
    errno_t r = midiOutGetVolume((HMIDIOUT)0, &v);
    ui_midi_warn_if_error(r);
    *volume = (fp64_t)v / (fp64_t)0xFFFFFFFFU;
    return 0;
}

static errno_t ui_midi_set_volume(struct ui_midi* m, fp64_t volume) {
    posix_swear(posix_thread.id() == ui_app.tid);
    posix_swear(ui_midi.is_open(m) && ui_midi.is_playing(m));
    DWORD v = (DWORD)(volume * (fp64_t)0xFFFFFFFFU);
    const UINT n = midiOutGetNumDevs();
    // Handle to a MIDI Output Device
    HMIDIOUT h = (HMIDIOUT)(uintptr_t)(n - 1);
    errno_t r = n == 0 ? MCIERR_DEVICE_NOT_INSTALLED : midiOutSetVolume(h, v);
    ui_midi_warn_if_error(r);
    posix_fatal_if_error(r);
    return r;
}

static errno_t ui_midi_stop(struct ui_midi* m) {
    posix_swear(posix_thread.id() == ui_app.tid);
    posix_swear(ui_midi.is_open(m) && ui_midi.is_playing(m));
    struct ui_midi_implementation* mi = (struct ui_midi_implementation*)m;
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_STOP, 0, 0);
    ui_midi_warn_if_error(r);
    if (r == 0) { mi->playing = false; }
    return r;
}

static void ui_midi_close(struct ui_midi* m) {
    posix_swear(posix_thread.id() == ui_app.tid);
    posix_swear(ui_midi.is_open(m) && !ui_midi.is_playing(m));
    struct ui_midi_implementation* mi = (struct ui_midi_implementation*)m;
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
    ui_midi_warn_if_error(r);
    r = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_WAIT, 0);
    ui_midi_warn_if_error(r);
    posix_fatal_if_error(r, "sound card is unplugged on the fly?");
    memset(&mi->mop, 0x00, sizeof(mi->mop));
    mi->window = 0;
    ui_midi_remove_handler(m);
}

static bool ui_midi_is_open(struct ui_midi* m) {
    struct ui_midi_implementation* mi = (struct ui_midi_implementation*)m;
    return mi->window != 0;
}

static bool ui_midi_is_playing(struct ui_midi* m) {
    struct ui_midi_implementation* mi = (struct ui_midi_implementation*)m;
    return mi->playing;
}

struct ui_midi_if ui_midi = {
    .success    = MCI_NOTIFY_SUCCESSFUL,
    .failure    = MCI_NOTIFY_FAILURE,
    .aborted    = MCI_NOTIFY_ABORTED,
    .superseded = MCI_NOTIFY_SUPERSEDED,
    .error      = ui_midi_error,
    .open       = ui_midi_open,
    .play       = ui_midi_play,
    .rewind     = ui_midi_rewind,
    .get_volume = ui_midi_get_volume,
    .set_volume = ui_midi_set_volume,
    .stop       = ui_midi_stop,
    .is_open    = ui_midi_is_open,
    .is_playing = ui_midi_is_playing,
    .close      = ui_midi_close
};