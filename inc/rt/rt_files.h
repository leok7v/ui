#pragma once
#include "rt/rt_std.h"

rt_begin_c

// space for "short" 260 utf-16 characters filename in utf-8 format:
typedef struct rt_file_name_s { char s[1024]; } rt_file_name_t;

enum { rt_files_max_path = (int32_t)sizeof(rt_file_name_t) }; // *)

typedef struct rt_file_s rt_file_t;

typedef struct rt_files_stat_s {
    uint64_t created;
    uint64_t accessed;
    uint64_t updated;
    int64_t  size; // bytes
    int64_t  type; // device / folder / symlink
} rt_files_stat_t;

typedef struct rt_folder_s {
    uint8_t data[512]; // implementation specific
} rt_folder_t;

typedef struct {
    rt_file_t* const invalid; // (rt_file_t*)-1
    // rt_files_stat_t.type:
    int32_t const type_folder;
    int32_t const type_symlink;
    int32_t const type_device;
    // seek() methods:
    int32_t const seek_set;
    int32_t const seek_cur;
    int32_t const seek_end;
    // open() flags
    int32_t const o_rd; // read only
    int32_t const o_wr; // write only
    int32_t const o_rw; // != (o_rd | o_wr)
    int32_t const o_append;
    int32_t const o_create; // opens existing or creates new
    int32_t const o_excl;   // create fails if file exists
    int32_t const o_trunc;  // open always truncates to empty
    int32_t const o_sync;
    // known folders ids:
    struct { // known folders:
        int32_t const home;      // "c:\Users\<username>" or "/users/<username>"
        int32_t const desktop;
        int32_t const documents;
        int32_t const downloads;
        int32_t const music;
        int32_t const pictures;
        int32_t const videos;
        int32_t const shared;    // "c:\Users\Public"
        int32_t const bin;       // "c:\Program Files" aka "/bin" aka "/Applications"
        int32_t const data;      // "c:\ProgramData" aka "/var" aka "/data"
    } const folder;
    errno_t (*open)(rt_file_t* *file, const char* filename, int32_t flags);
    bool    (*is_valid)(rt_file_t* file); // checks both null and invalid
    errno_t (*seek)(rt_file_t* file, int64_t *position, int32_t method);
    errno_t (*stat)(rt_file_t* file, rt_files_stat_t* stat, bool follow_symlink);
    errno_t (*read)(rt_file_t* file, void* data, int64_t bytes, int64_t *transferred);
    errno_t (*write)(rt_file_t* file, const void* data, int64_t bytes, int64_t *transferred);
    errno_t (*flush)(rt_file_t* file);
    void    (*close)(rt_file_t* file);
    errno_t (*write_fully)(const char* filename, const void* data,
                           int64_t bytes, int64_t *transferred);
    bool (*exists)(const char* pathname); // does not guarantee any access writes
    bool (*is_folder)(const char* pathname);
    bool (*is_symlink)(const char* pathname);
    const char* (*basename)(const char* pathname); // c:\foo\bar.ext -> bar.ext
    errno_t (*mkdirs)(const char* pathname); // tries to deep create all folders in pathname
    errno_t (*rmdirs)(const char* pathname); // tries to remove folder and its subtree
    errno_t (*create_tmp)(char* file, int32_t count); // create temporary file
    errno_t (*chmod777)(const char* pathname); // and whole subtree new files and folders
    errno_t (*symlink)(const char* from, const char* to); // sym link "ln -s" **)
    errno_t (*link)(const char* from, const char* to); // hard link like "ln"
    errno_t (*unlink)(const char* pathname); // delete file or empty folder
    errno_t (*copy)(const char* from, const char* to); // allows overwriting
    errno_t (*move)(const char* from, const char* to); // allows overwriting
    errno_t (*cwd)(char* folder, int32_t count); // get current working dir
    errno_t (*chdir)(const char* folder); // set working directory
    const char* (*known_folder)(int32_t kf_id);
    const char* (*bin)(void);  // Windows: "c:\Program Files" Un*x: "/bin"
    const char* (*data)(void); // Windows: "c:\ProgramData" Un*x: /data or /var
    const char* (*tmp)(void);  // temporary folder (system or user)
    // There are better, native, higher performance ways to iterate thru
    // folders in Posix, Linux and Windows. The following is minimalistic
    // approach to folder content reading:
    errno_t (*opendir)(rt_folder_t* folder, const char* folder_name);
    const char* (*readdir)(rt_folder_t* folder, rt_files_stat_t* optional);
    void (*closedir)(rt_folder_t* folder);
    void (*test)(void);
} rt_files_if;

// *) rt_files_max_path is a compromise - way longer than Windows MAX_PATH of 260
// and somewhat shorter than 32 * 1024 Windows long path.
// Use with caution understanding that it is a limitation and where it is
// important heap may and should be used. Do not to rely on thread stack size
// (default: 1MB on Windows, Android Linux 64KB, 512 KB  on MacOS, Ubuntu 8MB)
//
// **) symlink on Win32 is only allowed in Admin elevated
//     processes and in Developer mode.

extern rt_files_if rt_files;

rt_end_c
