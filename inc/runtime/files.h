#pragma once
#include "manifest.h"

begin_c

typedef struct {
    errno_t (*write_fully)(const char* filename, const void* data,
                           int64_t bytes, int64_t *transferred);
    bool (*exists)(const char* pathname); // does not guarantee any access writes
    bool (*is_folder)(const char* pathname);
    errno_t (*mkdirs)(const char* pathname); // tries to deep create all folders in pathname
    errno_t (*rmdirs)(const char* pathname); // tries to remove folder and its subtree
    errno_t (*tmp)(char* folder, int32_t count); // create temporary folder
    errno_t (*chmod777)(const char* pathname); // and whole subtree new files and folders
    const char* (*folder_bin)(void);  // Windows: "c:\ProgramFiles" Un*x: "/bin"
    const char* (*folder_data)(void); // Windows: "c:\ProgramData" Un*x: /data
    // cwd() shall return absolute pathname of the current working directory or null
    const char* (*cwd)(char* wd, int32_t count);
    errno_t (*setcwd)(const char* wd); // set working directory
    errno_t (*symlink)(const char* from, const char* to); // sym link "ln -s" *)
    errno_t (*link)(const char* from, const char* to); // hard link like "ln"
    errno_t (*unlink)(const char* pathname); // delete file or empty folder
    errno_t (*copy)(const char* from, const char* to); // allows overwriting
    errno_t (*move)(const char* from, const char* to); // allows overwriting
    void (*test)(void);
} files_if;

// *) suymlink on Win32 is only allowed in Admin elevated
//    processes and in Developer mode.

extern files_if files;

end_c
