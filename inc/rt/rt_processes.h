#pragma once
#include "rt/rt_streams.h"

rt_begin_c

typedef struct {
    const char* command;
    rt_stream_if* in;
    rt_stream_if* out;
    rt_stream_if* err;
    uint32_t exit_code;
    fp64_t   timeout; // seconds
} rt_processes_child_t;

// Process name may be an the executable filename with
// full, partial or absent pathname.
// Case insensitive on Windows.

typedef struct {
    const char* (*name)(void); // argv[0] like but full path
    uint64_t  (*pid)(const char* name); // 0 if process not found
    errno_t   (*pids)(const char* name, uint64_t* pids/*[size]*/, int32_t size,
                      int32_t *count); // return 0, error or ERROR_MORE_DATA
    errno_t   (*nameof)(uint64_t pid, char* name, int32_t count); // pathname
    bool      (*present)(uint64_t pid);
    errno_t   (*kill)(uint64_t pid, fp64_t timeout_seconds);
    errno_t   (*kill_all)(const char* name, fp64_t timeout_seconds);
    bool      (*is_elevated)(void); // Is process running as root/ Admin / System?
    errno_t   (*restart_elevated)(void); // retuns error or exits on success
    errno_t   (*run)(rt_processes_child_t* child);
    errno_t   (*popen)(const char* command, int32_t *xc, rt_stream_if* output,
                       fp64_t timeout_seconds); // <= 0 infinite
    // popen() does NOT guarantee stream zero termination on errors
    errno_t  (*spawn)(const char* command); // spawn fully detached process
    void (*test)(void);
} rt_processes_if;

extern rt_processes_if rt_processes;

rt_end_c
