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

typedef struct {
    errno_t (*write_fully)(const char* filename, const void* data,
                           int64_t bytes, int64_t *transferred);
    bool (*exists)(const char* pathname); // does not guarantee any access writes
    bool (*is_folder)(const char* pathname);
    errno_t (*mkdirs)(const char* pathname); // tries to deep create all folders in pathname
    errno_t (*rmdirs)(const char* pathname); // tries to remove folder and its subtree
    errno_t (*create_temp_folder)(char* folder, int32_t count);
    errno_t (*add_acl_ace)(const void* obj, int32_t obj_type, int32_t sid, uint32_t permissions);
    errno_t (*chmod777)(const char* pathname); // and whole subtree new files and folders
    const char* (*folder_bin)(void);  // Windows: "c:\ProgramFiles" Un*x: "/bin"
    const char* (*folder_data)(void); // Windows: "c:\ProgramData" Un*x: /data
    // cwd() shall return absolute pathname of the current working directory or null
    const char* (*cwd)(char* wd, int32_t count);
    errno_t (*set_cwd)(const char* wd); // set working directory
    errno_t (*symlink)(const char* from, const char* to); // sym link "ln -s"
    errno_t (*link)(const char* from, const char* to); // hard link like "ln"
    errno_t (*unlink)(const char* pathname); // delete file or empty folder
    errno_t (*copy)(const char* from, const char* to); // allows overwriting
    errno_t (*move)(const char* from, const char* to); // allows overwriting
    void (*test)(void);
} files_if;

extern files_if files;

end_c
