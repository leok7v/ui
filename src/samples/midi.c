/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "single_file_lib/rt/rt.h"
#include "ui/ut_win32.h"
#include <mmsystem.h>
#include "midi.h"

#pragma comment(lib, "winmm")

static uint64_t tid; // mi is thread sensitive

rt_static_init(midi) {
    tid = rt_thread.id(); // main() thread
}

typedef struct midi_s_ {
    uintptr_t window;
    MCI_OPEN_PARMSA mop; // opaque
    MCI_PLAY_PARMS pp;
} midi_t_;

rt_static_assertion(sizeof(midi_t) >= sizeof(midi_t_) + sizeof(int64_t));

static void midi_warn_if_error_(int r, const char* call, const char* func,
        int line) {
    if (r != 0) {
        static char error[256];
        mciGetErrorString(r, error, rt_countof(error));
        rt_println("%s:%d %s", func, line, call);
        rt_println("%d - MCIERR_BASE: %d %s", r, r - MCIERR_BASE, error);
    }
}

#define midi_warn_if_error(r) do {                  \
    midi_warn_if_error_(r, #r, __func__, __LINE__); \
} while (0)

#define midi_fatal_if_error(call) do {                                  \
    int _r_ = call; midi_warn_if_error_(r, #call, __func__, __LINE__);  \
    rt_fatal_if_error(r);                                               \
} while (0)

static errno_t midi_open(midi_t* m, void* window, const char* filename) {
    midi_t_* mi = (midi_t_*)m;
    rt_assert(rt_thread.id() == tid);
    mi->window = (uintptr_t)window;
    mi->mop.dwCallback = mi->window;
    mi->mop.wDeviceID = (WORD)-1;
    mi->mop.lpstrDeviceType = (const char*)MCI_DEVTYPE_SEQUENCER;
    mi->mop.lpstrElementName = filename;
    mi->mop.lpstrAlias = null;
    errno_t r = mciSendCommandA(0, MCI_OPEN,
            MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID | MCI_OPEN_ELEMENT,
            (uintptr_t)&mi->mop);
    midi_warn_if_error(r);
    rt_assert(mi->mop.wDeviceID != -1);
    m->device_id = mi->mop.wDeviceID;
    return r;
}

static errno_t midi_play(midi_t* m) {
    midi_t_* mi = (midi_t_*)m;
    rt_assert(rt_thread.id() == tid);
    memset(&mi->pp, 0x00, sizeof(mi->pp));
    mi->pp.dwCallback = (uintptr_t)mi->window;
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_PLAY, MCI_NOTIFY,
        (uintptr_t)&mi->pp);
    midi_warn_if_error(r);
    return r;
}

static errno_t midi_stop(midi_t* m) {
    midi_t_* mi = (midi_t_*)m;
    rt_assert(rt_thread.id() == tid);
    errno_t r = mciSendCommandA(mi->mop.wDeviceID, MCI_STOP, 0, 0);
    midi_warn_if_error(r);
    return r;
}

static void midi_close(midi_t* m) {
    midi_t_* mi = (midi_t_*)m;
    errno_t r = mciSendCommandA(mi->mop.wDeviceID,
        MCI_CLOSE, MCI_WAIT, 0);
    midi_warn_if_error(r);
    r = mciSendCommandA(MCI_ALL_DEVICE_ID, MCI_CLOSE, MCI_WAIT, 0);
    midi_warn_if_error(r);
}

midi_if midi = {
    .notify = MM_MCINOTIFY,
    .successful = MCI_NOTIFY_SUCCESSFUL,
    .superseded = MCI_NOTIFY_SUPERSEDED,
    .aborted = MCI_NOTIFY_ABORTED,
    .failure = MCI_NOTIFY_FAILURE,
    .success = MCI_NOTIFY_SUCCESSFUL,
    .open = midi_open,
    .play = midi_play,
    .stop = midi_stop,
    .close = midi_close
};