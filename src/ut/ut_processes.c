#include "ut/ut.h"
#include "ut/ut_win32.h"

typedef struct rt_processes_pidof_lambda_s rt_processes_pidof_lambda_t;

typedef struct rt_processes_pidof_lambda_s {
    bool (*each)(rt_processes_pidof_lambda_t* p, uint64_t pid); // returns true to continue
    uint64_t* pids;
    size_t size;  // pids[size]
    size_t count; // number of valid pids in the pids
    fp64_t timeout;
    errno_t error;
} rt_processes_pidof_lambda_t;

static int32_t rt_processes_for_each_pidof(const char* pname, rt_processes_pidof_lambda_t* la) {
    char stack[1024]; // avoid alloca()
    int32_t n = rt_str.len(pname);
    rt_fatal_if(n + 5 >= rt_countof(stack), "name is too long: %s", pname);
    const char* name = pname;
    // append ".exe" if not present:
    if (!rt_str.iends(pname, ".exe")) {
        int32_t k = (int32_t)strlen(pname) + 5;
        char* exe = stack;
        rt_str.format(exe, k, "%s.exe", pname);
        name = exe;
    }
    const char* base = strrchr(name, '\\');
    if (base != null) {
        base++; // advance past "\\"
    } else {
        base = name;
    }
    uint16_t wn[1024];
    rt_fatal_if(strlen(base) >= rt_countof(wn), "name too long: %s", base);
    rt_str.utf8to16(wn, rt_countof(wn), base, -1);
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
        r = rt_heap.reallocate(null, (void**)&data, bytes, false);
        if (r == 0) {
            r = NtQuerySystemInformation(SystemProcessInformation, data, bytes, &bytes);
        } else {
            rt_assert(r == (errno_t)ERROR_NOT_ENOUGH_MEMORY);
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
                    char path[rt_files_max_path];
                    match = rt_processes.nameof(pid, path, rt_countof(path)) == 0 &&
                            rt_str.iends(path, name);
//                  rt_println("\"%s\" -> \"%s\" match: %d", name, path, match);
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
    if (data != null) { rt_heap.deallocate(null, data); }
    rt_assert(count <= (uint64_t)INT32_MAX);
    return (int32_t)count;
}

static errno_t rt_processes_nameof(uint64_t pid, char* name, int32_t count) {
    rt_assert(name != null && count > 0);
    errno_t r = 0;
    name[0] = 0;
    HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)pid);
    if (p != null) {
        r = rt_b2e(GetModuleFileNameExA(p, null, name, count));
        name[count - 1] = 0; // ensure zero termination
        rt_win32_close_handle(p);
    } else {
        r = ERROR_NOT_FOUND;
    }
    return r;
}

static bool rt_processes_present(uint64_t pid) {
    void* h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, (DWORD)pid);
    bool b = h != null;
    if (h != null) { rt_win32_close_handle(h); }
    return b;
}

static bool rt_processes_first_pid(rt_processes_pidof_lambda_t* lambda, uint64_t pid) {
    lambda->pids[0] = pid;
    return false;
}

static uint64_t rt_processes_pid(const char* pname) {
    uint64_t first[1] = {0};
    rt_processes_pidof_lambda_t lambda = {
        .each = rt_processes_first_pid,
        .pids = first,
        .size  = 1,
        .count = 0,
        .timeout = 0,
        .error = 0
    };
    rt_processes_for_each_pidof(pname, &lambda);
    return first[0];
}

static bool rt_processes_store_pid(rt_processes_pidof_lambda_t* lambda, uint64_t pid) {
    if (lambda->pids != null && lambda->count < lambda->size) {
        lambda->pids[lambda->count++] = pid;
    }
    return true; // always - need to count all
}

static errno_t rt_processes_pids(const char* pname, uint64_t* pids/*[size]*/,
        int32_t size, int32_t *count) {
    *count = 0;
    rt_processes_pidof_lambda_t lambda = {
        .each = rt_processes_store_pid,
        .pids = pids,
        .size = (size_t)size,
        .count = 0,
        .timeout = 0,
        .error = 0
    };
    *count = rt_processes_for_each_pidof(pname, &lambda);
    return (int32_t)lambda.count == *count ? 0 : ERROR_MORE_DATA;
}

static errno_t rt_processes_kill(uint64_t pid, fp64_t timeout) {
    DWORD milliseconds = timeout < 0 ? INFINITE : (DWORD)(timeout * 1000);
    enum { access = PROCESS_QUERY_LIMITED_INFORMATION |
                    PROCESS_TERMINATE | SYNCHRONIZE };
    rt_assert((DWORD)pid == pid); // Windows... HANDLE vs DWORD in different APIs
    errno_t r = ERROR_NOT_FOUND;
    HANDLE h = OpenProcess(access, 0, (DWORD)pid);
    if (h != null) {
        char path[rt_files_max_path];
        path[0] = 0;
        r = rt_b2e(TerminateProcess(h, ERROR_PROCESS_ABORTED));
        if (r == 0) {
            DWORD ix = WaitForSingleObject(h, milliseconds);
            r = rt_wait_ix2e(ix);
        } else {
            DWORD bytes = rt_countof(path);
            errno_t rq = rt_b2e(QueryFullProcessImageNameA(h, 0, path, &bytes));
            if (rq != 0) {
                rt_println("QueryFullProcessImageNameA(pid=%d, h=%p) "
                        "failed %s", pid, h, rt_strerr(rq));
            }
        }
        rt_win32_close_handle(h);
        if (r == ERROR_ACCESS_DENIED) { // special case
            rt_thread.sleep_for(0.015); // need to wait a bit
            HANDLE retry = OpenProcess(access, 0, (DWORD)pid);
            // process may have died before we have chance to terminate it:
            if (retry == null) {
                rt_println("TerminateProcess(pid=%d, h=%p, im=%s) "
                        "failed but zombie died after: %s",
                        pid, h, path, rt_strerr(r));
                r = 0;
            } else {
                rt_win32_close_handle(retry);
            }
        }
        if (r != 0) {
            rt_println("TerminateProcess(pid=%d, h=%p, im=%s) failed %s",
                pid, h, path, rt_strerr(r));
        }
    }
    if (r != 0) { errno = r; }
    return r;
}

static bool rt_processes_kill_one(rt_processes_pidof_lambda_t* lambda, uint64_t pid) {
    errno_t r = rt_processes_kill(pid, lambda->timeout);
    if (r != 0) { lambda->error = r; }
    return true; // keep going
}

static errno_t rt_processes_kill_all(const char* name, fp64_t timeout) {
    rt_processes_pidof_lambda_t lambda = {
        .each = rt_processes_kill_one,
        .pids = null,
        .size  = 0,
        .count = 0,
        .timeout = timeout,
        .error = 0
    };
    int32_t c = rt_processes_for_each_pidof(name, &lambda);
    return c == 0 ? ERROR_NOT_FOUND : lambda.error;
}

static bool rt_processes_is_elevated(void) { // Is process running as Admin / System ?
    BOOL elevated = false;
    PSID administrators_group = null;
    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY administrators_group_authority = SECURITY_NT_AUTHORITY;
    errno_t r = rt_b2e(AllocateAndInitializeSid(&administrators_group_authority, 2,
                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &administrators_group));
    if (r != 0) {
        rt_println("AllocateAndInitializeSid() failed %s", rt_strerr(r));
    }
    PSID system_ops = null;
    SID_IDENTIFIER_AUTHORITY system_ops_authority = SECURITY_NT_AUTHORITY;
    r = rt_b2e(AllocateAndInitializeSid(&system_ops_authority, 2,
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS,
            0, 0, 0, 0, 0, 0, &system_ops));
    if (r != 0) {
        rt_println("AllocateAndInitializeSid() failed %s", rt_strerr(r));
    }
    if (administrators_group != null) {
        r = rt_b2e(CheckTokenMembership(null, administrators_group, &elevated));
    }
    if (system_ops != null && !elevated) {
        r = rt_b2e(CheckTokenMembership(null, administrators_group, &elevated));
    }
    if (administrators_group != null) { FreeSid(administrators_group); }
    if (system_ops != null) { FreeSid(system_ops); }
    if (r != 0) {
        rt_println("failed %s", rt_strerr(r));
    }
    return elevated;
}

static errno_t rt_processes_restart_elevated(void) {
    errno_t r = 0;
    if (!rt_processes.is_elevated()) {
        const char* path = rt_processes.name();
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = path;
        sei.hwnd = null;
        sei.nShow = SW_NORMAL;
        r = rt_b2e(ShellExecuteExA(&sei));
        if (r == ERROR_CANCELLED) {
            rt_println("The user unable or refused to allow privileges elevation");
        } else if (r == 0) {
            rt_core.exit(0); // second copy of the app is running now
        }
    }
    return r;
}

static void rt_processes_close_pipes(STARTUPINFOA* si,
        HANDLE *read_out,
        HANDLE *read_err,
        HANDLE *write_in) {
    if (si->hStdOutput != INVALID_HANDLE_VALUE) { rt_win32_close_handle(si->hStdOutput); }
    if (si->hStdError  != INVALID_HANDLE_VALUE) { rt_win32_close_handle(si->hStdError);  }
    if (si->hStdInput  != INVALID_HANDLE_VALUE) { rt_win32_close_handle(si->hStdInput);  }
    if (*read_out != INVALID_HANDLE_VALUE) { rt_win32_close_handle(*read_out); }
    if (*read_err != INVALID_HANDLE_VALUE) { rt_win32_close_handle(*read_err); }
    if (*write_in != INVALID_HANDLE_VALUE) { rt_win32_close_handle(*write_in); }
}

static errno_t rt_processes_child_read(rt_stream_if* out, HANDLE pipe) {
    char data[32 * 1024]; // Temporary buffer for reading
    DWORD available = 0;
    errno_t r = rt_b2e(PeekNamedPipe(pipe, null, sizeof(data), null,
                                 &available, null));
    if (r != 0) {
        if (r != ERROR_BROKEN_PIPE) { // unexpected!
//          rt_println("PeekNamedPipe() failed %s", rt_strerr(r));
        }
        // process has exited and closed the pipe
        rt_assert(r == ERROR_BROKEN_PIPE);
    } else if (available > 0) {
        DWORD bytes_read = 0;
        r = rt_b2e(ReadFile(pipe, data, sizeof(data), &bytes_read, null));
//      rt_println("r: %d bytes_read: %d", r, bytes_read);
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

static errno_t rt_processes_child_write(rt_stream_if* in, HANDLE pipe) {
    errno_t r = 0;
    if (in != null) {
        uint8_t  memory[32 * 1024]; // Temporary buffer for reading
        uint8_t* data = memory;
        int64_t bytes_read = 0;
        in->read(in, data, sizeof(data), &bytes_read);
        while (r == 0 && bytes_read > 0) {
            DWORD bytes_written = 0;
            r = rt_b2e(WriteFile(pipe, data, (DWORD)bytes_read,
                             &bytes_written, null));
            rt_println("r: %d bytes_written: %d", r, bytes_written);
            rt_assert((int32_t)bytes_written <= bytes_read);
            data += bytes_written;
            bytes_read -= bytes_written;
        }
    }
    return r;
}

static errno_t rt_processes_run(rt_processes_child_t* child) {
    const fp64_t deadline = rt_clock.seconds() + child->timeout;
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
    errno_t ro = rt_b2e(CreatePipe(&read_out, &si.hStdOutput, &sa, 0));
    errno_t re = rt_b2e(CreatePipe(&read_err, &si.hStdError,  &sa, 0));
    errno_t ri = rt_b2e(CreatePipe(&si.hStdInput, &write_in,  &sa, 0));
    if (ro != 0 || re != 0 || ri != 0) {
        rt_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        if (ro != 0) { rt_println("CreatePipe() failed %s", rt_strerr(ro)); r = ro; }
        if (re != 0) { rt_println("CreatePipe() failed %s", rt_strerr(re)); r = re; }
        if (ri != 0) { rt_println("CreatePipe() failed %s", rt_strerr(ri)); r = ri; }
    }
    if (r == 0) {
        r = rt_b2e(CreateProcessA(null, rt_str.drop_const(child->command),
                null, null, true, CREATE_NO_WINDOW, null, null, &si, &pi));
        if (r != 0) {
            rt_println("CreateProcess() failed %s", rt_strerr(r));
            rt_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        }
    }
    if (r == 0) {
        // not relevant: stdout can be written in other threads
        rt_win32_close_handle(pi.hThread);
        pi.hThread = null;
        // need to close si.hStdO* handles on caller side so,
        // when the process closes handles of the pipes, EOF happens
        // on caller side with io result ERROR_BROKEN_PIPE
        // indicating no more data can be read or written
        rt_win32_close_handle(si.hStdOutput);
        rt_win32_close_handle(si.hStdError);
        rt_win32_close_handle(si.hStdInput);
        si.hStdOutput = INVALID_HANDLE_VALUE;
        si.hStdError  = INVALID_HANDLE_VALUE;
        si.hStdInput  = INVALID_HANDLE_VALUE;
        bool done = false;
        while (!done && r == 0) {
            if (child->timeout > 0 && rt_clock.seconds() > deadline) {
                r = rt_b2e(TerminateProcess(pi.hProcess, ERROR_SEM_TIMEOUT));
                if (r != 0) {
                    rt_println("TerminateProcess() failed %s", rt_strerr(r));
                } else {
                    done = true;
                }
            }
            if (r == 0) { r = rt_processes_child_write(child->in, write_in); }
            if (r == 0) { r = rt_processes_child_read(child->out, read_out); }
            if (r == 0) { r = rt_processes_child_read(child->err, read_err); }
            if (!done) {
                DWORD ix = WaitForSingleObject(pi.hProcess, 0);
                // ix == 0 means process has exited (or terminated)
                // r == ERROR_BROKEN_PIPE process closed one of the handles
                done = ix == WAIT_OBJECT_0 || r == ERROR_BROKEN_PIPE;
            }
            // to avoid tight loop 100% cpu utilization:
            if (!done) { rt_thread.yield(); }
        }
        // broken pipe actually signifies EOF on the pipe
        if (r == ERROR_BROKEN_PIPE) { r = 0; } // not an error
//      if (r != 0) { rt_println("pipe loop failed %s", rt_strerr(r));}
        DWORD xc = 0;
        errno_t rx = rt_b2e(GetExitCodeProcess(pi.hProcess, &xc));
        if (rx == 0) {
            child->exit_code = xc;
        } else {
            rt_println("GetExitCodeProcess() failed %s", rt_strerr(rx));
            if (r != 0) { r = rx; } // report earliest error
        }
        rt_processes_close_pipes(&si, &read_out, &read_err, &write_in);
        // expected never to fail
        rt_win32_close_handle(pi.hProcess);
    }
    return r;
}

typedef struct {
    rt_stream_if stream;
    rt_stream_if* output;
    errno_t error;
} rt_processes_io_merge_out_and_err_if;

static errno_t rt_processes_merge_write(rt_stream_if* stream, const void* data,
        int64_t bytes, int64_t* transferred) {
    if (transferred != null) { *transferred = 0; }
    rt_processes_io_merge_out_and_err_if* s =
        (rt_processes_io_merge_out_and_err_if*)stream;
    if (s->output != null && bytes > 0) {
        s->error = s->output->write(s->output, data, bytes, transferred);
    }
    return s->error;
}

static errno_t rt_processes_open(const char* command, int32_t *exit_code,
        rt_stream_if* output,  fp64_t timeout) {
    rt_not_null(output);
    rt_processes_io_merge_out_and_err_if merge_out_and_err = {
        .stream ={ .write = rt_processes_merge_write },
        .output = output,
        .error = 0
    };
    rt_processes_child_t child = {
        .command = command,
        .in = null,
        .out = &merge_out_and_err.stream,
        .err = &merge_out_and_err.stream,
        .exit_code = 0,
        .timeout = timeout
    };
    errno_t r = rt_processes.run(&child);
    if (exit_code != null) { *exit_code = (int32_t)child.exit_code; }
    uint8_t zero = 0; // zero termination
    merge_out_and_err.stream.write(&merge_out_and_err.stream, &zero, 1, null);
    if (r == 0 && merge_out_and_err.error != 0) {
        r = merge_out_and_err.error; // zero termination is not guaranteed
    }
    return r;
}

static errno_t rt_processes_spawn(const char* command) {
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
    r = rt_b2e(CreateProcessA(null, rt_str.drop_const(command), null, null,
            /*bInheritHandles:*/false, flags, null, null, &si, &pi));
    if (r == 0) { // Close handles immediately
        rt_win32_close_handle(pi.hProcess);
        rt_win32_close_handle(pi.hThread);
    } else {
        rt_println("CreateProcess() failed %s", rt_strerr(r));
    }
    return r;
}

static const char* rt_processes_name(void) {
    static char mn[rt_files_max_path];
    if (mn[0] == 0) {
        rt_fatal_win32err(GetModuleFileNameA(null, mn, rt_countof(mn)));
    }
    return mn;
}

#ifdef UT_TESTS

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                       \
    if (rt_debug.verbosity.level >= rt_debug.verbosity.trace) { \
        rt_println(__VA_ARGS__);                                   \
    }                                                           \
} while (0)

static void rt_processes_test(void) {
    #ifdef UT_TESTS // in alphabetical order
    const char* names[] = { "svchost", "RuntimeBroker", "conhost" };
    for (int32_t j = 0; j < rt_countof(names); j++) {
        int32_t size  = 0;
        int32_t count = 0;
        uint64_t* pids = null;
        errno_t r = rt_processes.pids(names[j], null, size, &count);
        while (r == ERROR_MORE_DATA && count > 0) {
            size = count * 2; // set of processes may change rapidly
            r = rt_heap.reallocate(null, (void**)&pids,
                                  (int64_t)sizeof(uint64_t) * (int64_t)size,
                                  false);
            if (r == 0) {
                r = rt_processes.pids(names[j], pids, size, &count);
            }
        }
        if (r == 0 && count > 0) {
            for (int32_t i = 0; i < count; i++) {
                char path[256] = {0};
                #pragma warning(suppress: 6011) // dereferencing null
                r = rt_processes.nameof(pids[i], path, rt_countof(path));
                if (r != ERROR_NOT_FOUND) {
                    rt_assert(r == 0 && path[0] != 0);
                    verbose("%6d %s %s", pids[i], path, rt_strerr(r));
                }
            }
        }
        rt_heap.deallocate(null, pids);
    }
    // test popen()
    int32_t xc = 0;
    char data[32 * 1024];
    rt_stream_memory_if output;
    rt_streams.write_only(&output, data, rt_countof(data));
    const char* cmd = "cmd /c dir 2>nul >nul";
    errno_t r = rt_processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    rt_streams.write_only(&output, data, rt_countof(data));
    cmd = "cmd /c dir \"folder that does not exist\\\"";
    r = rt_processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    rt_streams.write_only(&output, data, rt_countof(data));
    cmd = "cmd /c dir";
    r = rt_processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    rt_streams.write_only(&output, data, rt_countof(data));
    cmd = "cmd /c timeout 1";
    r = rt_processes.popen(cmd, &xc, &output.stream, 1.0E-9);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    #endif
}

#pragma pop_macro("verbose")

#else

static void rt_processes_test(void) { }

#endif

rt_processes_if rt_processes = {
    .pid                 = rt_processes_pid,
    .pids                = rt_processes_pids,
    .nameof              = rt_processes_nameof,
    .present             = rt_processes_present,
    .kill                = rt_processes_kill,
    .kill_all            = rt_processes_kill_all,
    .is_elevated         = rt_processes_is_elevated,
    .restart_elevated    = rt_processes_restart_elevated,
    .run                 = rt_processes_run,
    .popen               = rt_processes_open,
    .spawn               = rt_processes_spawn,
    .name                = rt_processes_name,
    .test                = rt_processes_test
};
