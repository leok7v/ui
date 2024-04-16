#pragma once

#include "util/manifest.h"
#include "util/args.h"
#include "util/atomics.h"
#include "util/clock.h"
#include "util/config.h"
#include "util/debug.h"
#include "util/events.h"
#include "util/files.h"
#include "util/folders.h"
#include "util/heap.h"
#include "util/loader.h"
#include "util/mem.h"
#include "util/mutexes.h"
#include "util/num.h"
#include "util/static.h"
#include "util/str.h"
#include "util/streams.h"
#include "util/processes.h"
#include "util/threads.h"
#include "util/vigil.h"

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

