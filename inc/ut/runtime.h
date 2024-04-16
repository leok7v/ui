#pragma once
#include "ut/ut.h"

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

