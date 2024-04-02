#pragma once
#include "manifest.h"

begin_c

typedef struct {
    // On Unix it is responsibility of the main() to assign these values
    int32_t c;      // argc
    const char** v; // argv[argc]
    const char** env;
    int (*option_index)(int argc, const char* argv[], const char* option);
    int (*remove_at)(int ix, int argc, const char* argv[]);
    /* argc=3 argv={"foo", "--verbose"} -> returns true; argc=1 argv={"foo"} */
    bool (*option_bool)(int *argc, const char* argv[], const char* option);
    /* argc=3 argv={"foo", "--n", "153"} -> value==153, true; argc=1 argv={"foo"}
       also handles negative values (e.g. "-153") and hex (e.g. 0xBADF00D)
    */
    bool (*option_int)(int *argc, const char* argv[], const char* option, int64_t *value);
    /* argc=3 argv={"foo", "--path", "bar"} -> returns "bar" argc=1 argv={"foo"} */
    const char* (*option_str)(int *argc, const char* argv[], const char* option);
    int (*parse)(const char* cl, const char** argv, char* buff);
    void (*test)(void);
} args_if;

extern args_if args;

end_c
