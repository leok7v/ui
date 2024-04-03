#include "runtime.h"
#include "win32.h"

begin_c

// Wide character string ("wcs" aka UTF16) "endswith"
// s1 and s2 may be null
// Allow to use pidof("foo.exe") as well as pidof("C:\\Program Files\\foo.exe")

static bool wcs_endswith_nocase(const wchar_t* s1, const wchar_t* s2) {
    size_t n1 = s1 == null ? 0 : wcslen(s1);
    size_t n2 = s2 == null ? 0 : wcslen(s2);
    return s1 != null && s2 != null &&
           n1 >= n2 && wcsicmp(s1 + n1 - n2, s2) == 0;
}

static bool str_endswith_nocase(const char* s1, const char* s2) {
    size_t n1 = s1 == null ? 0 : strlen(s1);
    size_t n2 = s2 == null ? 0 : strlen(s2);
    return s1 != null && s2 != null &&
           n1 >= n2 && stricmp(s1 + n1 - n2, s2) == 0;
}

typedef struct pidof_lambda_s pidof_lambdat_t;

typedef struct pidof_lambda_s {
    bool (*each)(pidof_lambdat_t* p, uintptr_t pid); // returns true to continue
    uintptr_t* pids;
    size_t size;  // pids[size]
    size_t count; // number of valid pids in the pids
    double timeout;
    errno_t error;
} pidof_lambda_t;

static int32_t for_each_pidof(const char* pname, pidof_lambda_t* la) {
    const char* name = pname;
    // append ".exe" if not present (mixed casing ".eXe" etc - not handled):
    if (!strendswith(pname, ".exe") && !strendswith(pname, ".EXE")) {
        int32_t k = (int32_t)strlen(pname) + 5;
        char* exe = stackalloc(k);
        str.sformat(exe, k, "%s.exe", pname);
        name = exe;
    }
    const char* last_name = strrchr(name, '\\');
    if (last_name != null) {
        last_name++; // advance past "\\"
    } else {
        last_name = name;
    }
    wchar_t* wname = utf8to16(last_name);
    size_t count = 0;
    uintptr_t pid = 0;
    byte* data = null;
    ULONG bytes = 0;
    errno_t r = NtQuerySystemInformation(SystemProcessInformation, data, 0, &bytes);
    #pragma push_macro("STATUS_INFO_LENGTH_MISMATCH")
    #define STATUS_INFO_LENGTH_MISMATCH      0xC0000004
    while (r == STATUS_INFO_LENGTH_MISMATCH) {
        // bytes == 420768 on Windows 11 which may be a bit
        // too much for stack alloca()
        // add little extra if new process is spawned in between calls.
        bytes += sizeof(SYSTEM_PROCESS_INFORMATION) * 32;
        if (data == null) {
            data = (byte*)malloc(bytes);
        } else {
            byte* reallocated = (byte*)realloc(data, bytes);
            if (reallocated != null) { data = reallocated; }
        }
        if (data != null) {
            r = NtQuerySystemInformation(SystemProcessInformation, data, bytes, &bytes);
        } else {
            r = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    #pragma pop_macro("STATUS_INFO_LENGTH_MISMATCH")
    if (r == 0 && data != null) {
        SYSTEM_PROCESS_INFORMATION* proc = (SYSTEM_PROCESS_INFORMATION*)data;
        while (proc != null) {
            wchar_t* img = proc->ImageName.Buffer; // last name only, not a pathname!
            bool match = img != null && wcsicmp(img, wname) == 0;
            if (match) {
                pid = (uintptr_t)proc->UniqueProcessId; // HANDLE .UniqueProcessId
                if (last_name != name) {
                    char path[MAX_PATH];
                    match = processes.nameof(pid, path, countof(path)) == 0 &&
                        str_endswith_nocase(path, name);
//                  traceln("\"%s\" -> \"%s\" match: %d", name, path, match);
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
                ((byte*)proc + proc->NextEntryOffset) : null;
        }
    }
    if (data != null) { free(data); }
    assert((int32_t)count == count);
    return (int32_t)count;
}

static void processes_close_handle(HANDLE h) {
    fatal_if_false(CloseHandle(h));
}

static errno_t nameof(uintptr_t pid, char* name, int32_t count) {
    assert(name != null && count > 0);
    errno_t r = 0;
    name[0] = 0;
    HANDLE p = OpenProcess(PROCESS_ALL_ACCESS, false, (DWORD)pid);
    if (p != null) {
        r = b2e(GetModuleFileNameExA(p, null, name, count));
        name[count - 1] = 0; // ensure zero termination
        processes_close_handle(p);
    } else {
        r = ERROR_NOT_FOUND;
    }
    return r;
}

static bool present(uintptr_t pid) {
    void* h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, (DWORD)pid);
    bool b = h != null;
    if (h != null) { processes_close_handle(h); }
    return b;
}

static bool first_pid(pidof_lambda_t* lambda, uintptr_t pid) {
    lambda->pids[0] = pid;
    return false;
}

static uintptr_t pid(const char* pname) {
    uintptr_t first[1] = {0};
    pidof_lambda_t lambda = {
        .each = first_pid,
        .pids = first,
        .size  = 1,
        .count = 0,
        .timeout = 0,
        .error = 0
    };
    for_each_pidof(pname, &lambda);
    return first[0];
}

static bool store_pid(pidof_lambda_t* lambda, uintptr_t pid) {
    if (lambda->pids != null && lambda->count < lambda->size) {
        lambda->pids[lambda->count++] = pid;
    }
    return true; // always - need to count all
}

static errno_t pids(const char* pname, uintptr_t* pids/*[size]*/, int32_t size,
        int32_t *count) {
    *count = 0;
    pidof_lambda_t lambda = {
        .each = store_pid,
        .pids = pids,
        .size  = size,
        .count = 0,
        .timeout = 0,
        .error = 0
    };
    *count = for_each_pidof(pname, &lambda);
    return (int32_t)lambda.count == *count ? 0 : ERROR_MORE_DATA;
}

static errno_t processes_kill(uintptr_t pid, double timeout) {
    DWORD timeout_milliseconds = (DWORD)(timeout * 1000);
    enum { access = PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | SYNCHRONIZE };
    assert((DWORD)pid == pid); // Windows... HANDLE vs DWORD in different APIs
    errno_t r = ERROR_NOT_FOUND;
    HANDLE h = OpenProcess(access, 0, (DWORD)pid);
    if (h != null) {
        char path[MAX_PATH];
        path[0] = 0;
        r = b2e(TerminateProcess(h, ERROR_PROCESS_ABORTED));
        if (r == 0) {
            r = WaitForSingleObject(h, timeout_milliseconds);
            switch (r) {
                case WAIT_OBJECT_0 : r = 0;         break;
                case WAIT_ABANDONED: r = EINTR;     break;
                case WAIT_TIMEOUT  : r = ETIMEDOUT; break;
                case WAIT_FAILED   : r = GetLastError(); break;
                default: assert(false, "unexpected: %d", r); r = EINVAL; break;
            }
        } else {
            DWORD bytes = countof(path);
            errno_t rq = b2e(QueryFullProcessImageNameA(h, 0, path, &bytes));
            if (rq != 0) {
                traceln("QueryFullProcessImageNameA(pid=%d, h=%p) "
                        "failed %s", pid, h, str.error(rq));
            }
        }
        processes_close_handle(h);
        if (r == ERROR_ACCESS_DENIED) { // special case
            threads.sleep_for(0.015); // need to wait a bit
            HANDLE retry = OpenProcess(access, 0, (DWORD)pid);
            // process may have died before we have chance to terminate it:
            if (retry == null) {
                traceln("TerminateProcess(pid=%d, h=%p, im=%s) "
                        "failed but zombie died after: %s",
                        pid, h, path, str.error(r));
                r = 0;
            } else {
                processes_close_handle(retry);
            }
        }
        if (r != 0) {
            traceln("TerminateProcess(pid=%d, h=%p, im=%s) failed %s",
                pid, h, path, str.error(r));
        }
    }
    if (r != 0) { errno = r; }
    return r;
}

static bool processes_kill_one(pidof_lambda_t* lambda, uintptr_t pid) {
    errno_t r = processes_kill(pid, lambda->timeout);
    if (r != 0) { lambda->error = r; }
    return true; // keep going
}

static errno_t processes_kill_all(const char* name, double timeout) {
    pidof_lambda_t lambda = {
        .each = processes_kill_one,
        .pids = null,
        .size  = 0,
        .count = 0,
        .timeout = timeout,
        .error = 0
    };
    int32_t c = for_each_pidof(name, &lambda);
    return c == 0 ? ERROR_NOT_FOUND : lambda.error;
}

static bool processes_is_elevated(void) { // Is process running as Admin / System ?
    BOOL elevated = false;
    PSID administrators_group = null;
    // Allocate and initialize a SID of the administrators group.
    SID_IDENTIFIER_AUTHORITY administrators_group_authority = SECURITY_NT_AUTHORITY;
    errno_t r = b2e(AllocateAndInitializeSid(&administrators_group_authority, 2,
                SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                0, 0, 0, 0, 0, 0, &administrators_group));
    if (r != 0) {
        traceln("AllocateAndInitializeSid() failed %s", str.error(r));
    }
    PSID system_ops = null;
    SID_IDENTIFIER_AUTHORITY system_ops_authority = SECURITY_NT_AUTHORITY;
    r = b2e(AllocateAndInitializeSid(&system_ops_authority, 2,
            SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS,
            0, 0, 0, 0, 0, 0, &system_ops));
    if (r != 0) {
        traceln("AllocateAndInitializeSid() failed %s", str.error(r));
    }
    if (administrators_group != null) {
        r = b2e(CheckTokenMembership(null, administrators_group, &elevated));
    }
    if (system_ops != null && !elevated) {
        r = b2e(CheckTokenMembership(null, administrators_group, &elevated));
    }
    if (administrators_group != null) { FreeSid(administrators_group); }
    if (system_ops != null) { FreeSid(system_ops); }
    if (r != 0) {
        traceln("failed %s", str.error(r));
    }
    return elevated;
}

static errno_t processes_restart_elevated(void) {
    errno_t r = 0;
    if (!processes.is_elevated()) {
        const char* path = processes.name();
        SHELLEXECUTEINFOA sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = path;
        sei.hwnd = null;
        sei.nShow = SW_NORMAL;
        r = b2e(ShellExecuteExA(&sei));
        if (r == ERROR_CANCELLED) {
            traceln("The user unable or refused to allow privileges elevation");
        } else if (r == 0) {
            runtime.exit(0); // second copy of the app is running now
        }
    }
    return r;
}

static void processes_close_pipes(STARTUPINFOA* si,
        HANDLE *read_out,
        HANDLE *read_err,
        HANDLE *write_in) {
    if (si->hStdOutput != INVALID_HANDLE_VALUE) { processes_close_handle(si->hStdOutput); }
    if (si->hStdError  != INVALID_HANDLE_VALUE) { processes_close_handle(si->hStdError);  }
    if (si->hStdInput  != INVALID_HANDLE_VALUE) { processes_close_handle(si->hStdInput);  }
    if (*read_out != INVALID_HANDLE_VALUE) { processes_close_handle(*read_out); }
    if (*read_err != INVALID_HANDLE_VALUE) { processes_close_handle(*read_err); }
    if (*write_in != INVALID_HANDLE_VALUE) { processes_close_handle(*write_in); }
}

static errno_t processes_child_read(stream_if* out, HANDLE pipe) {
    char data[32 * 1024]; // Temporary buffer for reading
    DWORD available = 0;
    errno_t r = PeekNamedPipe(pipe, null, sizeof(data), null, &available, null) ?
            0 : runtime.err();
    if (r != 0) {
        if (r != ERROR_BROKEN_PIPE) { // unexpected!
//          traceln("PeekNamedPipe() failed %s", str.error(r));
        }
        // process has exited and closed the pipe
        assert(r == ERROR_BROKEN_PIPE);
    } else if (available > 0) {
        DWORD bytes_read = 0;
        r = ReadFile(pipe, data, sizeof(data), &bytes_read, null) ?
                0 : runtime.err();
//      traceln("r: %d bytes_read: %d", r, bytes_read);
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

static errno_t processes_child_write(stream_if* in, HANDLE pipe) {
    errno_t r = 0;
    if (in != null) {
        uint8_t  memory[32 * 1024]; // Temporary buffer for reading
        uint8_t* data = memory;
        int64_t bytes_read = 0;
        in->read(in, data, sizeof(data), &bytes_read);
        while (r == 0 && bytes_read > 0) {
            DWORD bytes_written = 0;
            r = WriteFile(pipe, data, (DWORD)bytes_read,
                            &bytes_written, null) ?
                            0 : runtime.err();
            traceln("r: %d bytes_written: %d", r, bytes_written);
            assert((int32_t)bytes_written <= bytes_read);
            data += bytes_written;
            bytes_read -= bytes_written;
        }
    }
    return r;
}

static errno_t processes_run(processes_child_t* child) {
    const double deadline = clock.seconds() + child->timeout;
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
    errno_t ro = CreatePipe(&read_out, &si.hStdOutput, &sa, 0) ? 0 : runtime.err();
    errno_t re = CreatePipe(&read_err, &si.hStdError,  &sa, 0) ? 0 : runtime.err();
    errno_t ri = CreatePipe(&si.hStdInput, &write_in,  &sa, 0) ? 0 : runtime.err();
    if (ro != 0 || re != 0 || ri != 0) {
        processes_close_pipes(&si, &read_out, &read_err, &write_in);
        if (ro != 0) { traceln("CreatePipe() failed %s", str.error(ro)); r = ro; }
        if (re != 0) { traceln("CreatePipe() failed %s", str.error(re)); r = re; }
        if (ri != 0) { traceln("CreatePipe() failed %s", str.error(ri)); r = ri; }
    }
    if (r == 0) {
        r = CreateProcessA(null, (char*)child->command, null, null, true,
                CREATE_NO_WINDOW, null, null, &si, &pi) ? 0: runtime.err();
        if (r != 0) {
            traceln("CreateProcess() failed %s", str.error(r));
            processes_close_pipes(&si, &read_out, &read_err, &write_in);
        }
    }
    if (r == 0) {
        // not relevant: stdout can be written in other threads
        fatal_if_false(CloseHandle(pi.hThread));
        pi.hThread = null;
        // need to close si.hStdO* handles on caller side so,
        // when the process closes handles of the pipes, EOF happens
        // on caller side with io result ERROR_BROKEN_PIPE
        // indicating no more data can be read or written
        fatal_if_false(CloseHandle(si.hStdOutput));
        fatal_if_false(CloseHandle(si.hStdError));
        fatal_if_false(CloseHandle(si.hStdInput));
        si.hStdOutput = INVALID_HANDLE_VALUE;
        si.hStdError  = INVALID_HANDLE_VALUE;
        si.hStdInput  = INVALID_HANDLE_VALUE;
        bool done = false;
        while (!done && r == 0) {
            if (child->timeout > 0 && clock.seconds() > deadline) {
                r = TerminateProcess(pi.hProcess, ERROR_SEM_TIMEOUT) ? 0 : runtime.err();
                if (r != 0) {
                    traceln("TerminateProcess() failed %s", str.error(r));
                } else {
                    done = true;
                }
            }
            if (r == 0) { r = processes_child_write(child->in, write_in); }
            if (r == 0) { r = processes_child_read(child->out, read_out); }
            if (r == 0) { r = processes_child_read(child->err, read_err); }
            if (!done) {
                DWORD ix = WaitForSingleObject(pi.hProcess, 0);
                // ix == 0 means process has exited (or terminated)
                // r == ERROR_BROKEN_PIPE process closed one of the handles
                done = ix == WAIT_OBJECT_0 || r == ERROR_BROKEN_PIPE;
            }
            // to avoid tight loop 100% cpu utilization:
            if (!done) { threads.yield(); }
        }
        // broken pipe actually signifies EOF on the pipe
        if (r == ERROR_BROKEN_PIPE) { r = 0; } // not an error
//      if (r != 0) { traceln("pipe loop failed %s", str.error(r));}
        DWORD xc = 0;
        errno_t rx = GetExitCodeProcess(pi.hProcess, &xc) ? 0 : runtime.err();
        if (rx == 0) {
            child->exit_code = xc;
        } else {
            traceln("GetExitCodeProcess() failed %s", str.error(rx));
            if (r != 0) { r = rx; } // report earliest error
        }
        processes_close_pipes(&si, &read_out, &read_err, &write_in);
        fatal_if_false(CloseHandle(pi.hProcess)); // expected never to fail
    }
    return r;
}

typedef struct processes_io_merge_out_and_err_if {
    stream_if stream;
    stream_if* output;
    errno_t error;
} processes_io_merge_out_and_err_if;

static errno_t processes_merge_write(stream_if* stream, const void* data,
        int64_t bytes, int64_t* transferred) {
    if (transferred != null) { *transferred = 0; }
    processes_io_merge_out_and_err_if* s =
        (processes_io_merge_out_and_err_if*)stream;
    if (s->output != null && bytes > 0) {
        s->error = s->output->write(s->output, data, bytes, transferred);
    }
    return s->error;
}

static errno_t processes_open(const char* command, int32_t *exit_code,
        stream_if* output,  double timeout) {
    not_null(output);
    processes_io_merge_out_and_err_if merge_out_and_err = {
        .stream ={ .write = processes_merge_write },
        .output = output,
        .error = 0
    };
    processes_child_t child = {
        .command = command,
        .in = null,
        .out = &merge_out_and_err.stream,
        .err = &merge_out_and_err.stream,
        .exit_code = 0,
        .timeout = timeout
    };
    errno_t r = processes.run(&child);
    if (exit_code != null) { *exit_code = child.exit_code; }
    uint8_t zero = 0; // zero termination
    merge_out_and_err.stream.write(&merge_out_and_err.stream, &zero, 1, null);
    if (r == 0 && merge_out_and_err.error != 0) {
        r = merge_out_and_err.error; // zero termination is not guaranteed
    }
    return r;
}

static errno_t processes_spawn(const char* command) {
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
    const static DWORD flags = CREATE_BREAKAWAY_FROM_JOB
                | CREATE_NO_WINDOW
                | CREATE_NEW_PROCESS_GROUP
                | DETACHED_PROCESS;
    PROCESS_INFORMATION pi = {0};
    r = CreateProcessA(null, (char*)command, null, null,
            /*bInheritHandles:*/false,
            flags, null, null, &si, &pi) ? 0: runtime.err();
    if (r == 0) { // Close handles immediately
        fatal_if_false(CloseHandle(pi.hProcess));
        fatal_if_false(CloseHandle(pi.hThread));
    } else {
//      traceln("CreateProcess() failed %s", str.error(r));
    }
    return r;
}

static const char* processes_name(void) {
    static char module_name[MAX_PATH];
    if (module_name[0] == 0) {
        fatal_if_false(GetModuleFileNameA(null, module_name, countof(module_name)));
    }
    return module_name;
}

#pragma push_macro("verbose") // --verbosity trace

#define verbose(...) do {                                 \
    if (debug.verbosity.level >= debug.verbosity.trace) { \
        traceln(__VA_ARGS__);                             \
    }                                                     \
} while (0)

static void processes_test(void) {
    #ifdef RUNTIME_TESTS // in alphabetical order
    const char* names[] = { "svchost", "RuntimeBroker", "conhost" };
    for (int32_t j = 0; j < countof(names); j++) {
        int32_t size  = 0;
        int32_t count = 0;
        uintptr_t* pids = null;
        errno_t r = processes.pids(names[j], null, size, &count);
        while (r == ERROR_MORE_DATA && count > 0) {
            size = count * 2; // set of processes may change rapidly
            pids = stackalloc(sizeof(uintptr_t) * size);
            r = processes.pids(names[j], pids, size, &count);
        }
        if (r == 0 && count > 0) {
            for (int32_t i = 0; i < count; i++) {
                char path[256] = {0};
                #pragma warning(suppress: 6011) // dereferencing null
                r = processes.nameof(pids[i], path, countof(path));
                if (r != ERROR_NOT_FOUND) {
                    assert(r == 0 && !strempty(path));
                    verbose("%6d %s %s", pids[i], path, r == 0 ? "" : str.error(r));
                }
            }
        }
    }
    // test popen()
    int32_t xc = 0;
    char data[32 * 1024];
    stream_memory_if output;
    streams.write_only(&output, data, countof(data));
    const char* cmd = "cmd /c dir 2>nul >nul";;
    errno_t r = processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    streams.write_only(&output, data, countof(data));
    cmd = "cmd /c dir \"folder that does not exist\\\"";
    r = processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    streams.write_only(&output, data, countof(data));
    cmd = "cmd /c dir";
    r = processes.popen(cmd, &xc, &output.stream, 99999.0);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    streams.write_only(&output, data, countof(data));
    cmd = "cmd /c timeout 1";
    r = processes.popen(cmd, &xc, &output.stream, 1.0E-9);
    verbose("r: %d xc: %d output:\n%s", r, xc, data);
    #endif
}

#pragma pop_macro("verbose")

processes_if processes = {
    .pid                 = pid,
    .pids                = pids,
    .nameof              = nameof,
    .present             = present,
    .kill                = processes_kill,
    .kill_all            = processes_kill_all,
    .is_elevated         = processes_is_elevated,
    .restart_elevated    = processes_restart_elevated,
    .run                 = processes_run,
    .popen               = processes_open,
    .spawn               = processes_spawn,
    .name                = processes_name,
    .test                = processes_test
};

end_c
