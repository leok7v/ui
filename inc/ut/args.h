#pragma once
#include "ut/std.h"

begin_c

typedef struct {
    // On Unix it is responsibility of the main() to assign these values
    int32_t c;      // argc
    const char** v; // argv[argc]
    const char** env;
    int32_t (*option_index)(int32_t argc, const char* argv[],
             const char* option); // e.g. option: "--verbosity" or "-v"
    int32_t (*remove_at)(int32_t ix, int32_t argc, const char* argv[]);
    /* argc=3 argv={"foo", "--verbose"} -> returns true; argc=1 argv={"foo"} */
    bool (*option_bool)(int32_t *argc, const char* argv[], const char* option);
    /* argc=3 argv={"foo", "--n", "153"} -> value==153, true; argc=1 argv={"foo"}
       also handles negative values (e.g. "-153") and hex (e.g. 0xBADF00D)
    */
    bool (*option_int)(int32_t *argc, const char* argv[], const char* option,
                       int64_t *value);
    // for argc=3 argv={"foo", "--path", "bar"}
    //     option_str("--path", option)
    // returns option: "bar" and argc=1 argv={"foo"} */
    const char* (*option_str)(int32_t *argc, const char* argv[],
                              const char* option);
    int32_t (*parse)(const char* cl, const char** argv, char* buff);
    void (*test)(void);
} args_if;

extern args_if args;

end_c
