#include "ut/ut.h"
#include "ut/ut_win32.h"

void ut_win32_close_handle(void* h) {
    #pragma warning(suppress: 6001) // shut up overzealous IntelliSense
    ut_fatal_win32err(CloseHandle((HANDLE)h));
}
