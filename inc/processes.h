#pragma once
#include "manifest.h"
#include "streams.h"

begin_c

typedef struct {
    const char* command;
    stream_if* in;
    stream_if* out;
    stream_if* err;
    uint32_t exit_code;
    double   timeout; // seconds
} processes_child_t;

// Process name could be just the executable filename or
// full or partial pathname. Case insensitive on Windows.

typedef struct {
    const char* (*name)(void); // argv[0] like but full path
    uintptr_t (*pid)(const char* name); // 0 if process not found
    int  (*pids)(const char* name, uintptr_t* pids/*[size]*/, int32_t size,
         int32_t *count); // return 0, error or ERROR_MORE_DATA
    int  (*nameof)(uintptr_t pid, char* name, int32_t count); // pathname
    bool (*present)(uintptr_t pid);
    int  (*kill)(uintptr_t pid, double timeout_seconds);
    int  (*kill_all)(const char* name, double timeout_seconds);
    bool (*is_elevated)(void); // Is process running as root/ Admin / System?
    int  (*restart_elevated)(void); // retuns error or exits on success
    int  (*run)(processes_child_t* child);
    int  (*popen)(const char* command, int32_t *exit_code,
                  stream_if* output, double timeout_seconds); // <= 0 infinite
    // popen() does NOT guarantee stream zero termination on errors
    int  (*spawn)(const char* command); // spawn fully detached process
    void (*test)(void);
} processes_if;

extern processes_if processes;

end_c