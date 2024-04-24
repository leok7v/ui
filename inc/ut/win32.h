#pragma once
#ifdef WIN32

#include <Windows.h>  // used by:
#include <psapi.h>    // both loader.c and processes.c
#include <shellapi.h> // processes.c
#include <winternl.h> // processes.c
#include <initguid.h>     // for knownfolders
#include <knownfolders.h> // files.c
#include <aclapi.h>       // files.c
#include <shlobj_core.h>  // files.c
#include <shlwapi.h>      // files.c
#include <fcntl.h>

#define export __declspec(dllexport)

#define b2e(call) (call ? 0 : GetLastError()) // BOOL -> errno_t

#define wait2e(ix) (errno_t)                                                     \
    ((int32_t)WAIT_OBJECT_0 <= (int32_t)(ix) && (ix) <= WAIT_OBJECT_0 + 63 ? 0 : \
      ((ix) == WAIT_ABANDONED ? ERROR_REQUEST_ABORTED :                          \
        ((ix) == WAIT_TIMEOUT ? ERROR_TIMEOUT :                                  \
          ((ix) == WAIT_FAILED) ? (errno_t)GetLastError() : ERROR_INVALID_HANDLE \
        )                                                                        \
      )                                                                          \
    )


#endif // WIN32
