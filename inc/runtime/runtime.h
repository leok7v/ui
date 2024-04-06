#pragma once

#include "runtime/manifest.h"
#include "runtime/args.h"
#include "runtime/atomics.h"
#include "runtime/clock.h"
#include "runtime/config.h"
#include "runtime/debug.h"
#include "runtime/events.h"
#include "runtime/files.h"
#include "runtime/folders.h"
#include "runtime/heap.h"
#include "runtime/loader.h"
#include "runtime/mem.h"
#include "runtime/mutexes.h"
#include "runtime/num.h"
#include "runtime/static.h"
#include "runtime/str.h"
#include "runtime/streams.h"
#include "runtime/processes.h"
#include "runtime/threads.h"
#include "runtime/vigil.h"

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

