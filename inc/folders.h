#pragma once
#include "manifest.h"

begin_c

typedef struct folders_s folders_t;

typedef struct folders_if {
    folders_t* (*open)(void);
    errno_t (*enumerate)(folders_t* folders, const char* folder);
    // name of the last enumerated folder
    const char* (*folder_name)(folders_t* folders);
    // number of enumerated files and sub folders inside folder
    int32_t (*count)(folders_t* folders);
    // name() of the [i]-th enumerated entry (folder or file) (not pathname!)
    const char* (*name)(folders_t* folders, int32_t i);
    bool (*is_folder)(folders_t* folders, int32_t i);
    bool (*is_symlink)(folders_t* folders, int32_t i);
    int64_t (*size)(folders_t* folders, int32_t i);
    // functions created/updated/accessed() return time in absolute nanoseconds
    // since start of OS epoch or 0 if failed or not available
    uint64_t (*created)(folders_t* folders, int32_t i);
    uint64_t (*updated)(folders_t* folders, int32_t i);
    uint64_t (*accessed)(folders_t* folders, int32_t i);
    void (*close)(folders_t* folders);
    void (*test)(void);
} folders_if;

extern folders_if folders;

end_c
