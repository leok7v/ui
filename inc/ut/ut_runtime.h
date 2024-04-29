#pragma once
#include "ut/ut_std.h"

begin_c

typedef struct {
    int32_t (*err)(void); // errno or GetLastError()
    void (*seterr)(int32_t err); // errno = err or SetLastError()
    void (*abort)(void);
    void (*exit)(int32_t exit_code); // only 8 bits on posix
    void (*test)(void);
} ut_runtime_if;

extern ut_runtime_if ut_runtime;

end_c

