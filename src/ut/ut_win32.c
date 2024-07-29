#include "ut/ut.h"
#include "ut/ut_win32.h"

void rt_win32_close_handle(void* h) {
    #pragma warning(suppress: 6001) // shut up overzealous IntelliSense
    rt_fatal_win32err(CloseHandle((HANDLE)h));
}

// WAIT_ABANDONED only reported for mutexes not events
// WAIT_FAILED means event was invalid handle or was disposed
// by another thread while the calling thread was waiting for it.

/* translate ix to error */
errno_t rt_wait_ix2e(uint32_t r) {
    const int32_t ix = (int32_t)r;
    return (errno_t)(
          (int32_t)WAIT_OBJECT_0 <= ix && ix <= WAIT_OBJECT_0 + 63 ? 0 :
          (ix == WAIT_ABANDONED ? ERROR_REQUEST_ABORTED :
            (ix == WAIT_TIMEOUT ? ERROR_TIMEOUT :
              (ix == WAIT_FAILED) ? rt_core.err() : ERROR_INVALID_HANDLE
            )
          )
    );
}