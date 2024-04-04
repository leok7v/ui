#pragma once

#include "manifest.h"
#include "args.h"
#include "atomics.h"
#include "clock.h"
#include "debug.h"
#include "dotfiles.h"
#include "events.h"
#include "files.h"
#include "folders.h"
#include "heap.h"
#include "loader.h"
#include "mem.h"
#include "mutexes.h"
#include "num.h"
#include "static.h"
#include "str.h"
#include "streams.h"
#include "processes.h"
#include "threads.h"
#include "vigil.h"

#define crt_version 20240318 // YYYYMMDD

begin_c

typedef struct {
    int32_t (*err)(void); // errno or GetLastError()
    void (*seterr)(int32_t err); // errno = err or SetLastError()
    void (*abort)(void);
    void (*exit)(int32_t exit_code); // only 8 bits on posix
    void (*test)(void);
} runtime_if;

extern runtime_if runtime;

end_c

