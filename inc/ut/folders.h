#pragma once
#include "ut/std.h"

begin_c

typedef struct folder_s folder_t;

typedef struct folders_if {
    const char* (*bin)(void);  // Windows: "c:\ProgramFiles" Un*x: "/bin"
    const char* (*data)(void); // Windows: "c:\ProgramData" Un*x: /data or /var
    const char* (*tmp)(void);  // temporary folder (system or user)
    // cwd() shall return absolute pathname of the current working directory or null
    errno_t (*getcwd)(char* folder, int32_t count);
    errno_t (*chdir)(const char* folder); // set working directory
    errno_t (*opendir)(folder_t* *folders, const char* folder);
    const char* (*folder)(folder_t* folders); // name of the folder
    // number of enumerated files and sub folders inside folder
    int32_t (*count)(folder_t* folders);
    // name() of the [i]-th enumerated entry (folder or file) (not a pathname!)
    const char* (*name)(folder_t* folders, int32_t i);
    bool (*is_folder)(folder_t* folders, int32_t i);
    bool (*is_symlink)(folder_t* folders, int32_t i);
    int64_t (*bytes)(folder_t* folders, int32_t i);
    // functions created/updated/accessed() return time in absolute nanoseconds
    // since start of OS epoch or 0 if failed or not available
    uint64_t (*created)(folder_t* folders, int32_t i);
    uint64_t (*updated)(folder_t* folders, int32_t i);
    uint64_t (*accessed)(folder_t* folders, int32_t i);
    void (*closedir)(folder_t* folders);
    void (*test)(void);
} folders_if;

extern folders_if folders;

end_c
