#pragma once
#ifdef WIN32

#include <Windows.h>  // used by:
#include <psapi.h>    // both loader.c and processes.c
#include <shellapi.h> // processes.c
#include <winternl.h> // processes.c
#include <immintrin.h> // _tzcnt_u32 num.c
#include <initguid.h>     // for knownfolders
#include <knownfolders.h> // files.c
#include <aclapi.h>       // files.c
#include <shlobj_core.h>  // files.c
#include <shlwapi.h>      // files.c

#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <malloc.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

// Microsoft runtime debug heap:
#if (defined(_DEBUG) || defined(DEBUG)) && !defined(_malloca)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h> // _malloca()
#endif

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
