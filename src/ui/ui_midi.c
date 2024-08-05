/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "rt/rt.h"
#include "rt/rt_win32.h"
#include "ui/ui.h"
#include <mmsystem.h>

#pragma comment(lib, "winmm")

typedef struct ui_midi_s_ {
    MCI_OPEN_PARMSA mop; // opaque
    ui_app_message_handler_t handler;
    char alias[32];
    int64_t device_id;
    uintptr_t window;
    bool playing;
    bool paused;
} ui_midi_t_;

rt_static_assertion(sizeof(ui_midi_t) >= sizeof(ui_midi_t_) + sizeof(void*));
rt_static_assertion(MMSYSERR_NOERROR == 0);

static void ui_midi_warn_if_error_(int r, const char* call, const char* func,
        int line) {
    if (r != 0) {
        static char error[256];
        mciGetErrorString(r, error, rt_countof(error));
        rt_println("%s:%d %s", func, line, call);
        rt_println("%d - MCIERR_BASE: %d %s", r, r - MCIERR_BASE, error);
    }
}

#define ui_midi_warn_if_error(r) do {                  \
    ui_midi_warn_if_error_(r, #r, __func__, __LINE__); \
} while (0)

#define ui_midi_fatal_if_error(call) do {                                   \
    int _r_ = call; ui_midi_warn_if_error_(r, #call, __func__, __LINE__);   \
    rt_fatal_if_error(r);                                                   \
} while (0)

static bool ui_midi_message_callback(ui_app_message_handler_t* h, int32_t m,
                                     int64_t wp, int64_t lp, int64_t* rt) {
    if (m == MM_MCINOTIFY) {
#ifdef UI_MIDI_DEBUG
        rt_println("device_id: %lld", lp);
        if (wp & MCI_NOTIFY_SUCCESSFUL) {
            rt_println("MCI_NOTIFY_SUCCESSFUL");
        }
        if (wp & MCI_NOTIFY_SUPERSEDED) {
            rt_println("MCI_NOTIFY_SUPERSEDED");
        }
        if (wp & MCI_NOTIFY_ABORTED) {
            rt_println("MCI_NOTIFY_ABORTED");
        }
        if (wp & MCI_NOTIFY_FAILURE) {
            rt_println("MCI_NOTIFY_FAILURE");
        }
#endif
        ui_midi_t* midi = (ui_midi_t*)h->that;
        ui_midi_t_* mi  = (ui_midi_t_*)midi;
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

static void ui_midi_remove_handler(ui_midi_t* m) {
    ui_midi_t_* mi  = (ui_midi_t_*)m;
    ui_app_message_handler_t* h = ui_app.handlers;
    if (h == &mi->handler) {
        ui_app.handlers = h->next;
    } else {
        while (h->next != null && h->next != &mi->handler) {
            h = h->next;
        }
        rt_swear(h->next == &mi->handler);
        if (h->next == &mi->handler) {
            h->next = h->next->next;
        }
    }
    mi->handler.callback = null;
    mi->handler.that = null;
    mi->handler.next = null;
}

static errno_t ui_midi_open(ui_midi_t* m, const char* filename) {
    rt_swear(rt_thread.id() == ui_app.tid);
    ui_midi_t_* mi = (ui_midi_t_*)m;
    mi->handler.that = mi;
    mi->handler.next = ui_app.handlers;
    ui_app.handlers = &mi->handler;
    mi->window = (uintptr_t)ui_app.window;
    mi->playing = false;
    mi->paused  = false;
    mi->mop.dwCallback = mi->window;
    mi->mop.wDeviceID = (WORD)-1;
    mi->mop.lpstrDeviceType = (const char*)MCI_DEVTYPE_SEQUENCER;
    mi->mop.lpstrElementName = filename;
    mi->mop.lpstrAlias = mi->alias;
    rt_str_printf(mi->alias, "%p", m);
    const DWORD_PTR flags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
                            MCI_OPEN_ELEMENT | MCI_OPEN_ALIAS;
    errno_t r = mciSendCommandA(0, MCI_OPEN, flags, (uintptr_t)&mi->mop);
    ui_midi_warn_if_error(r);
    rt_assert(mi->mop.wDeviceID != -1);
    mi->handler.callback = ui_midi_message_callback,
    mi->device_id = mi->mop.wDeviceID;
    if (r != 0) {
        ui_midi_remove_handler(m);
        memset(&mi->mop, 0x00, sizeof(mi->mop));
        mi->window = 0;
    }
    return r;
}

static errno_t ui_midi_play(ui_midi_t* m) {
    rt_swear(rt_thread.id() == ui_app.tid);
    ui_midi_t_* mi = (ui_midi_t_*)m;
    rt_swear(ui_midi.is_open(m));
    MCI_PLAY_PARMS  pp = { .dwCallback = (uintptr_t)mi->window };
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_PLAY, MCI_NOTIFY, (uintptr_t)&pp);
    ui_midi_warn_if_error(r);
    if (r == 0) {
        mi->playing = true;
        mi->paused  = false;
    }
    return r;
}

static errno_t ui_midi_pause(ui_midi_t* m) {
    rt_swear(rt_thread.id() == ui_app.tid);
    rt_swear(ui_midi.is_open(m) && ui_midi.is_playing(m) && !ui_midi.is_paused(m));
    ui_midi_t_* mi = (ui_midi_t_*)m;
    MCI_GENERIC_PARMS p = { .dwCallback = (uintptr_t)mi->window };
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_PAUSE, MCI_WAIT, (DWORD_PTR)&p);
    ui_midi_warn_if_error(r);
    if (r == 0) { mi->paused = true; }
    return r;
}

static errno_t ui_midi_resume(ui_midi_t* m) {
    rt_swear(rt_thread.id() == ui_app.tid);
    rt_swear(ui_midi.is_open(m) && ui_midi.is_playing(m) && ui_midi.is_paused(m));
    ui_midi_t_* mi = (ui_midi_t_*)m;
    MCI_GENERIC_PARMS p = { .dwCallback = (uintptr_t)mi->window };
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_RESUME, MCI_WAIT, (DWORD_PTR)&p);
    ui_midi_warn_if_error(r);
    if (r == 0) { mi->paused = false; }
    return r;
}

static errno_t ui_midi_rewind(ui_midi_t* m) {
    rt_swear(rt_thread.id() == ui_app.tid);
    rt_swear(ui_midi.is_open(m));
    ui_midi_t_* mi = (ui_midi_t_*)m;
    MCI_SEEK_PARMS p = { .dwCallback = (uintptr_t)mi->window, .dwTo = 0 };
    const DWORD f = MCI_WAIT|MCI_SEEK_TO_START;
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_SEEK, f, (DWORD_PTR)&p);
    ui_midi_warn_if_error(r);
    if (r == 0) { mi->paused = false; }
    return r;
}

static errno_t ui_midi_get_volume(ui_midi_t* m, fp64_t* volume) {
    rt_swear(rt_thread.id() == ui_app.tid);
    rt_swear(ui_midi.is_open(m) && ui_midi.is_playing(m));
    DWORD v = 0;
    errno_t r = midiOutGetVolume((HMIDIOUT)0, &v);
    ui_midi_warn_if_error(r);
    *volume = (fp64_t)v / (fp64_t)0xFFFFFFFFU;
    return 0;
}

static errno_t ui_midi_set_volume(ui_midi_t* m, fp64_t volume) {
    rt_swear(rt_thread.id() == ui_app.tid);
    rt_swear(ui_midi.is_open(m) && ui_midi.is_playing(m));
    DWORD v = (DWORD)(volume * (fp64_t)0xFFFFFFFFU);
    errno_t r = midiOutSetVolume((HMIDIOUT)0, v);
    ui_midi_warn_if_error(r);
    rt_fatal_if_error(r);
    return r;
}

static errno_t ui_midi_stop(ui_midi_t* m) {
    rt_swear(rt_thread.id() == ui_app.tid);
    rt_swear(ui_midi.is_open(m) && ui_midi.is_playing(m));
    ui_midi_t_* mi = (ui_midi_t_*)m;
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_STOP, 0, 0);
    ui_midi_warn_if_error(r);
    if (r == 0) { mi->playing = false; }
    return r;
}

static void ui_midi_close(ui_midi_t* m) {
    rt_swear(rt_thread.id() == ui_app.tid);
    rt_swear(ui_midi.is_open(m) && !ui_midi.is_playing(m));
    ui_midi_t_* mi = (ui_midi_t_*)m;
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_CLOSE, MCI_WAIT, 0);
    ui_midi_warn_if_error(r);
    r = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_WAIT, 0);
    ui_midi_warn_if_error(r);
    rt_fatal_if_error(r, "sound card is unplugged on the fly?");
    memset(&mi->mop, 0x00, sizeof(mi->mop));
    mi->window = 0;
    ui_midi_remove_handler(m);
}

static bool ui_midi_is_open(ui_midi_t* m) {
    ui_midi_t_* mi = (ui_midi_t_*)m;
    return mi->window != 0;
}

static bool ui_midi_is_playing(ui_midi_t* m) {
    ui_midi_t_* mi = (ui_midi_t_*)m;
    return mi->playing;
}

static bool ui_midi_is_paused(ui_midi_t* m) {
    ui_midi_t_* mi = (ui_midi_t_*)m;
    return mi->paused;
}

ui_midi_if ui_midi = {
    .successful = MCI_NOTIFY_SUCCESSFUL,
    .superseded = MCI_NOTIFY_SUPERSEDED,
    .aborted    = MCI_NOTIFY_ABORTED,
    .failure    = MCI_NOTIFY_FAILURE,
    .success    = MCI_NOTIFY_SUCCESSFUL,
    .open       = ui_midi_open,
    .play       = ui_midi_play,
    .pause      = ui_midi_pause,
    .resume     = ui_midi_resume,
    .rewind     = ui_midi_rewind,
    .get_volume = ui_midi_get_volume,
    .set_volume = ui_midi_set_volume,
    .stop       = ui_midi_stop,
    .is_open    = ui_midi_is_open,
    .is_playing = ui_midi_is_playing,
    .is_paused  = ui_midi_is_paused,
    .close      = ui_midi_close
};