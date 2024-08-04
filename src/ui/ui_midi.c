/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "rt/rt.h"
#include "rt/rt_win32.h"
#include "ui/ui.h"
#include <mmsystem.h>

#pragma comment(lib, "winmm")

static uint64_t tid; // mi is thread sensitive

rt_static_init(midi) {
    tid = rt_thread.id(); // main() thread
}

typedef struct ui_midi_s_ {
    uintptr_t window;
    MCI_OPEN_PARMSA mop; // opaque
    MCI_PLAY_PARMS pp;
    int64_t device_id;
    ui_app_message_handler_t handler;
} ui_midi_t_;

rt_static_assertion(sizeof(ui_midi_t) >= sizeof(ui_midi_t_) + sizeof(int64_t));

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
    ui_midi_t_* mi = (ui_midi_t_*)m;
    rt_assert(rt_thread.id() == tid);
    mi->handler.that = mi;
    mi->handler.next = ui_app.handlers;
    ui_app.handlers = &mi->handler;
    mi->window = (uintptr_t)ui_app.window;
    mi->mop.dwCallback = mi->window;
    mi->mop.wDeviceID = (WORD)-1;
    mi->mop.lpstrDeviceType = (const char*)MCI_DEVTYPE_SEQUENCER;
    mi->mop.lpstrElementName = filename;
    mi->mop.lpstrAlias = null;
    const DWORD_PTR flags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_OPEN_ELEMENT;
    errno_t r = mciSendCommandA(0, MCI_OPEN, flags, (uintptr_t)&mi->mop);
    ui_midi_warn_if_error(r);
    rt_assert(mi->mop.wDeviceID != -1);
    mi->handler.callback = ui_midi_message_callback,
    mi->device_id = mi->mop.wDeviceID;
    if (r != 0) {
        ui_midi_remove_handler(m);
    }
    return r;
}

static errno_t ui_midi_play(ui_midi_t* m) {
    ui_midi_t_* mi = (ui_midi_t_*)m;
    rt_assert(rt_thread.id() == tid);
    memset(&mi->pp, 0x00, sizeof(mi->pp));
    mi->pp.dwCallback = (uintptr_t)mi->window;
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_PLAY, MCI_NOTIFY,
        (uintptr_t)&mi->pp);
    ui_midi_warn_if_error(r);
    return r;
}

static errno_t ui_midi_stop(ui_midi_t* m) {
    ui_midi_t_* mi = (ui_midi_t_*)m;
    rt_assert(rt_thread.id() == tid);
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_STOP, 0, 0);
    ui_midi_warn_if_error(r);
    return r;
}

static void ui_midi_close(ui_midi_t* m) {
    ui_midi_t_* mi = (ui_midi_t_*)m;
    errno_t r = mciSendCommandA(mi->mop.wDeviceID,
        MCI_CLOSE, MCI_WAIT, 0);
    ui_midi_warn_if_error(r);
    r = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_WAIT, 0);
    ui_midi_warn_if_error(r);
    ui_midi_remove_handler(m);
}

ui_midi_if ui_midi = {
    .successful = MCI_NOTIFY_SUCCESSFUL,
    .superseded = MCI_NOTIFY_SUPERSEDED,
    .aborted    = MCI_NOTIFY_ABORTED,
    .failure    = MCI_NOTIFY_FAILURE,
    .success    = MCI_NOTIFY_SUCCESSFUL,
    .open       = ui_midi_open,
    .play       = ui_midi_play,
    .stop       = ui_midi_stop,
    .close      = ui_midi_close
};