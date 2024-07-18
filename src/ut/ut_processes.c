#include "ut/ut.h"
#include "ut/ut_win32.h"

typedef struct ut_processes_pidof_lambda_s ut_processes_pidof_lambda_t;

typedef struct ut_processes_pidof_lambda_s {
    bool (*each)(ut_processes_pidof_lambda_t* p, uint64_t pid); // returns true to continue
    uint64_t* pids;
    size_t size;  // pids[size]
    size_t count; // number of valid pids in the pids
    fp64_t timeout;
    errno_t error;
} ut_processes_pidof_lambda_t;

static int32_t ut_processes_for_each_pidof(const char* pname, ut_processes_pidof_lambda_t* la) {
    char stack[1024]; // avoid alloca()
    int32_t n = ut_str.len(pname);
    ut_fatal_if(n + 5 >= ut_countof(stack), "name is too long: %s", pname);
    const char* name = pname;
    // append ".exe" if not present:
    if (!ut_str.iends(pname, ".exe")) {
        int32_t k = (int32_t)strlen(pname) + 5;
        char* exe = stack;
        ut_str.format(exe, k, "%s.exe", pname);
        name = exe;
    }
    const char* base = strrchr(name, '\\');
    if (base != null) {
        base++; // advance past "\\"
    } else {
        base = name;
    }
    uint16_t wn[1024];
    ut_fatal_if(strlen(base) >= ut_countof(wn), "name too long: %s", base);
    ut_str.utf8to16(wn, ut_countof(wn), base, -1);
    size_t count = 0;
    uint64_t pid = 0;
    uint8_t* data = null;
    ULONG bytes = 0;
    errno_t r = NtQuerySystemInformation(SystemProcessInformation, data, 0, &bytes);
    #pragma push_macro("STATUS_INFO_LENGTH_MISMATCH")
    #define STATUS_INFO_LENGTH_MISMATCH      0xC0000004
    while (r == (errno_t)STATUS_INFO_LENGTH_MISMATCH) {
        // bytes == 420768 on Windows 11 which may be a bit
        // too much for stack alloca()
        // add little extra if new process is spawned in between calls.
        bytes += sizeof(SYSTEM_PROCESS_INFORMATION) * 32;
        r = ut_heap.reallocate(null, (void**)&data, bytes, false);
        if (r == 0) {
            r = NtQuerySystemInformation(SystemProcessInformation, data, bytes, &bytes);
        } else {
            assert(r == (errno_t)ERROR_NOT_ENOUGH_MEMORY);
        }
    }
    #pragma pop_macro("STATUS_INFO_LENGTH_MISMATCH")
    if (r == 0 && data != null) {
        SYSTEM_PROCESS_INFORMATION* proc = (SYSTEM_PROCESS_INFORMATION*)data;
        while (proc != null) {
            uint16_t* img = proc->ImageName.Buffer; // last name only, not a pathname!
            bool match = img != null && wcsicmp(img, wn) == 0;
            if (match) {
                pid = (uint64_t)proc->UniqueProcessId; // HANDLE .UniqueProcessId
                if (base != name) {
                    char path[ut_files_max_path];
                    match = ut_processes.nameof(pid, path, ut_countof(path)) == 0 &&
                            ut_str.iends(path, name);
//                  ut_println("\"%s\" -> \"%s\" match: %d", name, path, match);
                }
            }
            if (match) {
                if (la != null && count < la->size && la->pids != null) {
                    la->pids[count] = pid;
                }
                count++;
                if (la != null && la->each != null && !la->each(la, pid)) {
                    break;
                }
            }
            proc = proc->NextEntryOffset != 0 ? (SYSTEM_PROCESS_INFORMATION*)
                ((uint8_t*)proc + proc->NextEntryOffset) : null;
        }
    }
    if (data != null) { ut_heap.deallocate(null, data); }
    assert(count <= (uint64_t)INT32_MAX);
    return (int32_t)count;
}

static errno_t ut_processes_nameof(uint64_t pid, char* name, int32_t count) {
    assert(name != null && count > 0);
    errno_t r = 0;
    name[0] = 0;
    HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)pid);
    if (p != null) {
        r = ut_b2e(GetModuleFileNameExA(p, null, name, count));
        name[count - 1] = 0; // ensure zero termination
        ut_win32_close_handle(p);
    } else {
        r = ERROR_NOT_FOUND;
    }
    return r;
}

static bool ut_processes_present(uint64_t pid) {
    void* h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, (DWORD)pid);
    bool b = h != null;
    if (h != null) { ut_win32_close_handle(h); }
    return b;
}

static bool ut_processes_first_pid(ut_processes_pidof_lambda_t* lambda, uint64_t pid) {
    lambda->pids[0] = pid;
    return false;
}

static uint64_t ut_processes_pid(const char* pname) {
    uint64_t first[1] = {0};
    ut_processes_pidof_lambda_t lambda = {
        .each = ut_processes_first_pid,
        .pids = first,
        .size  = 1,
        .count = 0,
        .timeout = 0,
        .error = 0
    };
    ut_processes_for_each_pidof(pname, &lambda);
    return first[0];
}

static bool ut_processes_store_pid(ut_processes_pidof_lambda_t* lambda, uint64_t pid) {
    if (lambda->pids != null && lambda->count < lambda->size) {
        lambda->pids[lambda->count++] = pid;
    }
    return true; // always - need to count all
}

static errno_t ut_processes_pids(const char* pname, uint64_t* pids/*[size]*/,
        int32_t size, int32_t *count) {
    *count = 0;
    ut_processes_pidof_lambda_t lambda = {
        .each = ut_processes_store_pid,
        .pids = pids,
        .size = (size_t)size,
        .count = 0,
        .timeout = 0,
        .error = 0
    };
    *count = ut_processes_for_each_pidof(pname, &lambda);
    return (int32_t)lambda.count == *count ? 0 : ERROR_MORE_DATA;
}

static errno_t ut_processes_kill(uint64_t pid, fp64_t timeout) {
    DWORD milliseconds = timeout < 0 ? INFINITE : (DWORD)(timeout * 1000);
    enum { access = PROCESS_QUERY_LIMITED_INFORMATION |
                    PROCESS_TERMINATE | SYNCHRONIZE };
    assert((DWORD)pid == pid); // Windows... HANDLE vs DWORD in different APIs
    errno_t r = ERROR_NOT_FOUND;
    HANDLE h = OpenProcess(access, 0, (DWORD)pid);
    if (h != null) {
        char path[ut_files_max_path];
        path[0] = 0;
        r = ut_b2e(TerminateProcess(h, ERROR_PROCESS_ABORTED));
        if (r == 0) {
            DWORD ix = WaitForSingleObject(h, milliseconds);
            r = ut_wait_ix2e(ix);
        } else {
            DWORD bytes = ut_countof(path);
            errno_t rq = ut_b2e(QueryFullProcessImageNameA(h, 0, path, &bytes));
            if (rq != 0) {
                ut_println("QueryFullProcessImageNameA(pid=%d, h=%p) "
                        "failed %s", pid, h, ut_strerr(rq));
            }
        }
        ut_win32_close_handle(h);
        if (r == ERROR_ACCESS_DENIED) { // special case
            ut_thread.sleep_for(0.015); // need to wait a bit
            HANDLE retry = OpenProcess(access, 0, (DWORD)pid);
            // process may have died before we have chance to terminate it:
            if (retry == null) {
                ut_println("TerminateProcess(pid=%d, h=%p, im=%s) "
                        "failed but zombie died after: %s",
                        pid, h, path, ut_strerr(r));
                r = 0;
            } else {
                ut_win32_close_handle(retry);
            }
        }
        if (r != 0) {
            ut_println("TerminateProcess(pid=%d, h=%p, im=%s) failed %s",
                pid, h, path, ut_strerr(r));
        }
    }
    if (r != 0) { errno = r; }
    return r;
}

static bool ut_processes_kill_one(ut_processes_pidof_lambda_t* lambda, uint64_t pid) {
    errno_t r = ut_processes_kill(pid, lambda->timeout);
    if (r != 0) { lambda->error = r; }
    return true; // keep going
}

static errno_t ut_processes_kill_all(const char* name, fp64_t timeout) {
    ut_processes_pidof_lambda_t lambda = {
        .each = ut_processes_kill_one,
        .pids = null,
        .size  = 0,
        .count = 0,
        .timeout = timeout,
        .error = 0
    };
    int32_t c = ut_processes_for_each_pidof(name, &lambda);
    return c == 0 ? ERROR_NOT_FOUND : lambda.error;
}

static bool ut_processes_is_elevated(void) { // Is process running as Admin / System ?
    BOOL elevated = false;
    PSID administrators_group = null;
    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY administrators_group_authority = SECURITY_NT_AUTHORITY;
    errno_t r = ut_b2e(AllocateAndInitializeSid(&administrators_group_authority, 2,
                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &administrators_group));
    if (r != 0) {
        ut_println("AllocateAndInitializeSid() failed %s", ut_strerr(r));
    }
    PSID system_ops = null;
    SID_IDENTIFIER_AUTHORITY system_ops_authority = SECURITY_NT_AUTHORITY;
    r = ut_b2e(AllocateAndInitializeSid(&system_ops_authority, 2,
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS,
            0, 0, 0, 0, 0, 0, &system_ops));
    if (r != 0) {
        ut_println("AllocateAndInitializeSid() failed %s", ut_strerr(r));
    }
    if (administrators_group != null) {
        r = ut_b2e(CheckTokenMembership(null, administrators_group, &elevated));
    }
    if (system_ops != null && !elevated) {
        r = ut_b2e(CheckTokenMembership(null, administrators_group, &elevated));
    }
    if (administrators_group != null) { FreeSid(administrators_group); }
    if (system_ops != null) { FreeSid(system_ops); }
    if (r != 0) {
        ut_println("failed %s", ut_strerr(r));
    }
    return elevated;
}

static errno_t ut_processes_restart_elevated(void) {
    errno_t r = 0;
    if (!ut_processes.is_elevated()) {
        const char* path = ut_processes.name();
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = path;
        sei.hwnd = null;
        sei.nShow = SW_NORMAL;
        r = ut_b2e(ShellExecuteExA(&sei));
        if (r == ERROR_CANCELLED) {
            ut_println("The user unable or refused to allow privileges elevation");
        } else if (r == 0) {
            ut_runtime.exit(0); // second copy of the app is running now
        }
    }
    return r;
}

static void ut_processes_close_pipes(STARTUPINFOA* si,
        HANDLE *read_out,
        HANDLE *read_err,
        HANDLE *write_in) {
    if (si->hStdOutput != INVALID_HANDLE_VALUE) { ut_win32_close_handle(si->hStdOutput); }
    if (si->hStdError  != INVALID_HANDLE_VALUE) { ut_win32_close_handle(si->hStdError);  }
    if (si->hStdInput  != INVALID_HANDLE_VALUE) { ut_win32_close_handle(si->hStdInput);  }
    if (*read_out != INVALID_HANDLE_VALUE) { ut_win32_close_handle(*read_out); }
    if (*read_err != INVALID_HANDLE_VALUE) { ut_win32_close_handle(*read_err); }
    if (*write_in != INVALID_HANDLE_VALUE) { ut_win32_close_handle(*write_in); }
}

static errno_t ut_processes_child_read(ut_stream_if* out, HANDLE pipe) {
    char data[32 * 1024]; // Temporary buffer for reading
    DWORD available = 0;
    errno_t r = ut_b2e(PeekNamedPipe(pipe, null, sizeof(data), null,
                                 &available, null));
    if (r != 0) {
        if (r != ERROR_BROKEN_PIPE) { // unexpected!
//          ut_println("PeekNamedPipe() failed %s", ut_strerr(r));
        }
        // process has exited and closed the pipe
        assert(r == ERROR_BROKEN_PIPE);
    } else if (available > 0) {
        DWORD bytes_read = 0;
        r = ut_b2e(ReadFile(pipe, data, sizeof(data), &bytes_read, null));
//      ut_println("r: %d bytes_read: %d", r, bytes_read);
        if (out != null) {
            if (r == 0) {
                r = out->write(out, data, bytes_read, null);
            }
        } else {
            // no one interested - drop on the floor
        }
    }
    return r;
}

static errno_t ut_processes_child_write(ut_stream_if* in, HANDLE pipe) {
    errno_t r = 0;
    if (in != null) {
        uint8_t  memory[32 * 1024]; // Temporary buffer for reading
        uint8_t* data = memory;
        int64_t bytes_read = 0;
        in->read(in, data, sizeof(data), &bytes_read);
        while (r == 0 && bytes_read > 0) {
            DWORD bytes_written = 0;
            r = ut_b2e(WriteFile(pipe, data, (DWORD)bytes_read,
                             &bytes_written, null));
            ut_println("r: %d bytes_written: %d", r, bytes_written);
            assert((int32_t)bytes_written <= bytes_read);
            data += bytes_written;
            bytes_read -= bytes_written;
        }
    }
    return r;
}

static errno_t ut_processes_run(ut_processes_child_t* child) {
    const fp64_t deadline = ut_clock.seconds() + child->timeout;
    errno_t r = 0;
    STARTUPINFOA si = {
        .cb = sizeof(STARTUPINFOA),
        .hStdInput  = INVALID_HANDLE_VALUE,
        .hStdOutput = INVALID_HANDLE_VALUE,
        .hStdError  = INVALID_HANDLE_VALUE,
        .dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES,
        .wShowWindow = SW_HIDE
    };
    SECURITY_ATTRIBUTES sa = { sizeof(sa), null, true };  // Inheritable handles
    PROCESS_INFORMATION pi = {0};
    HANDLE read_out = INVALID_HANDLE_VALUE;
    HANDLE read_err = INVALID_HANDLE_VALUE;
    HANDLE write_in = INVALID_HANDLE_VALUE;
    errno_t ro = ut_b2e(CreatePipe(&read_out, &si.hStdOutput, &sa, 0));
    errno_t re = ut_b2e(CreatePipe(&read_err, &si.hStdError,  &sa, 0));
    errno_t ri = ut_b2e(CreatePipe(&si.hStdInput, &write_in,  &sa, 0));
    if (ro != 0 || re != 0 || ri != 0) {
        ut_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        if (ro != 0) { ut_println("CreatePipe() failed %s", ut_strerr(ro)); r = ro; }
        if (re != 0) { ut_println("CreatePipe() failed %s", ut_strerr(re)); r = re; }
        if (ri != 0) { ut_println("CreatePipe() failed %s", ut_strerr(ri)); r = ri; }
    }
    if (r == 0) {
        r = ut_b2e(CreateProcessA(null, ut_str.drop_const(child->command),
                null, null, true, CREATE_NO_WINDOW, null, null, &si, &pi));
        if (r != 0) {
            ut_println("CreateProcess() failed %s", ut_strerr(r));
            ut_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        }
    }
    if (r == 0) {
        // not relevant: stdout can be written in other threads
        ut_win32_close_handle(pi.hThread);
        pi.hThread = null;
        // need to close si.hStdO* handles on caller side so,
        // when the process closes handles of the pipes, EOF happens
        // on caller side with io result ERROR_BROKEN_PIPE
        // indicating no more data can be read or written
        ut_win32_close_handle(si.hStdOutput);
        ut_win32_close_handle(si.hStdError);
        ut_win32_close_handle(si.hStdInput);
        si.hStdOutput = INVALID_HANDLE_VALUE;
        si.hStdError  = INVALID_HANDLE_VALUE;
        si.hStdInput  = INVALID_HANDLE_VALUE;
        bool done = false;
        while (!done && r == 0) {
            if (child->timeout > 0 && ut_clock.seconds() > deadline) {
                r = ut_b2e(TerminateProcess(pi.hProcess, ERROR_SEM_TIMEOUT));
                if (r != 0) {
                    ut_println("TerminateProcess() failed %s", ut_strerr(r));
                } else {
                    done = true;
                }
            }
            if (r == 0) { r = ut_processes_child_write(child->in, write_in); }
            if (r == 0) { r = ut_processes_child_read(child->out, read_out); }
            if (r == 0) { r = ut_processes_child_read(child->err, read_err); }
            if (!done) {
                DWORD ix = WaitForSingleObject(pi.hProcess, 0);
                // ix == 0 means process has exited (or terminated)
                // r == ERROR_BROKEN_PIPE process closed one of the handles
                done = ix == WAIT_OBJECT_0 || r == ERROR_BROKEN_PIPE;
            }
            // to avoid tight loop 100% cpu utilization:
            if (!done) { ut_thread.yield(); }
        }
        // broken pipe actually signifies EOF on the pipe
        if (r == ERROR_BROKEN_PIPE) { r = 0; } // not an error
//      if (r != 0) { ut_println("pipe loop failed %s", ut_strerr(r));}
        DWORD xc = 0;
        errno_t rx = ut_b2e(GetExitCodeProcess(pi.hProcess, &xc));
        if (rx == 0) {
            child->exit_code = xc;
        } else {
            ut_println("GetExitCodeProcess() failed %s", ut_strerr(rx));
            if (r != 0) { r = rx; } // report earliest error
        }
        ut_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        // expected never to fail
        ut_win32_close_handle(pi.hProcess);
    }
    return r;
}

typedef struct {
    ut_stream_if stream;
    ut_stream_if* output;
    errno_t error;
} ut_processes_io_merge_out_and_err_if;

static errno_t ut_processes_merge_write(ut_stream_if* stream, const void* data,
        int64_t bytes, int64_t* transferred) {
    if (transferred != null) { *transferred = 0; }
    ut_processes_io_merge_out_and_err_if* s =
        (ut_processes_io_merge_out_and_err_if*)stream;
    if (s->output != null && bytes > 0) {
        s->error = s->output->write(s->output, data, bytes, transferred);
    }
    return s->error;
}

static errno_t ut_processes_open(const char* command, int32_t *exit_code,
        ut_stream_if* output,  fp64_t timeout) {
    ut_not_null(output);
    ut_processes_io_merge_out_and_err_if merge_out_and_err = {
        .stream ={ .write = ut_processes_merge_write },
        .output = output,
        .error = 0
    };
    ut_processes_child_t child = {
        .command = command,
        .in = null,
        .out = &merge_out_and_err.stream,
        .err = &merge_out_and_err.stream,
        .exit_code = 0,
        .timeout = timeout
    };
    errno_t r = ut_processes.run(&child);
    if (exit_code != null) { *exit_code = (int32_t)child.exit_code; }
    uint8_t zero = 0; // zero termination
    merge_out_and_err.stream.write(&merge_out_and_err.stream, &zero, 1, null);
    if (r == 0 && merge_out_and_err.error != 0) {
        r = merge_out_and_err.error; // zero termination is not guaranteed
    }
    return r;
}

static errno_t ut_processes_spawn(const char* command) {
    errno_t r = 0;
    STARTUPINFOA si = {
        .cb = sizeof(STARTUPINFOA),
        .dwFlags     = STARTF_USESHOWWINDOW
                     | CREATE_NEW_PROCESS_GROUP
                     | DETACHED_PROCESS,
        .wShowWindow = SW_HIDE,
        .hStdInput  = INVALID_HANDLE_VALUE,
        .hStdOutput = INVALID_HANDLE_VALUE,
        .hStdError  = INVALID_HANDLE_VALUE
    };
    const DWORD flags = CREATE_BREAKAWAY_FROM_JOB
                | CREATE_NO_WINDOW
                | CREATE_NEW_PROCESS_GROUP
                | DETACHED_PROCESS;
    PROCESS_INFORMATION pi = { .hProcess = null, .hThread = null };
    r = ut_b2e(CreateProcessA(null, ut_str.drop_const(command), null, null,
            /*bInheritHandles:*/false, flags, null, null, &si, &pi));
    if (r == 0) { // Close handles immediately
        ut_win32_close_handle(pi.hProcess);
        ut_win32_close_handle(pi.hThread);
    } else {
        ut_println("CreateProcess() failed %s", ut_strerr(r));
    }
    return r;
}

static const char* ut_processes_name(void) {
    static char mn[ut_files_max_path];
    if (mn[0] == 0) {
        ut_fatal_win32err(GetModuleFileNameA(null, mn, ut_countof(mn)));
    }
    return mn;
}

#ifdef UT_TESTS

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                       \
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) { \
        ut_println(__VA_ARGS__);                                   \
    }                                                           \
} while (0)

static void ut_processes_test(void) {
    #ifdef UT_TESTS // in alphabetical order
    const char* names[] = { "svchost", "RuntimeBroker", "conhost" };
    for (int32_t j = 0; j < ut_countof(names); j++) {
        int32_t size  = 0;
        int32_t count = 0;
        uint64_t* pids = null;
        errno_t r = ut_processes.pids(names[j], null, size, &count);
        while (r == ERROR_MORE_DATA && count > 0) {
            size = count * 2; // set of processes may change rapidly
            r = ut_heap.reallocate(null, (void**)&pids,
                                  (int64_t)sizeof(uint64_t) * (int64_t)size,
                                  false);
            if (r == 0) {
                r = ut_processes.pids(names[j], pids, size, &count);
            }
        }
        if (r == 0 && count > 0) {
            for (int32_t i = 0; i < count; i++) {
                char path[256] = {0};
                #pragma warning(suppress: 6011) // dereferencing null
                r = ut_processes.nameof(pids[i], path, ut_countof(path));
                if (r != ERROR_NOT_FOUND) {
                    assert(r == 0 && path[0] != 0);
                    verbose("%6d %s %s", pids[i], path, ut_strerr(r));
                }
            }
        }
        ut_heap.deallocate(null, pids);
    }
    // test popen()
    int32_t xc = 0;
    char data[32 * 1024];
    ut_stream_memory_if output;
    ut_streams.write_only(&output, data, ut_countof(data));
    const char* cmd = "cmd /c dir 2>nul >nul";
    errno_t r = ut_processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    ut_streams.write_only(&output, data, ut_countof(data));
    cmd = "cmd /c dir \"folder that does not exist\\\"";
    r = ut_processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    ut_streams.write_only(&output, data, ut_countof(data));
    cmd = "cmd /c dir";
    r = ut_processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    ut_streams.write_only(&output, data, ut_countof(data));
    cmd = "cmd /c timeout 1";
    r = ut_processes.popen(cmd, &xc, &output.stream, 1.0E-9);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    #endif
}

#pragma pop_macro("verbose")

#else

static void ut_processes_test(void) { }

#endif

ut_processes_if ut_processes = {
    .pid                 = ut_processes_pid,
    .pids                = ut_processes_pids,
    .nameof              = ut_processes_nameof,
    .present             = ut_processes_present,
    .kill                = ut_processes_kill,
    .kill_all            = ut_processes_kill_all,
    .is_elevated         = ut_processes_is_elevated,
    .restart_elevated    = ut_processes_restart_elevated,
    .run                 = ut_processes_run,
    .popen               = ut_processes_open,
    .spawn               = ut_processes_spawn,
    .name                = ut_processes_name,
    .test                = ut_processes_test
};
