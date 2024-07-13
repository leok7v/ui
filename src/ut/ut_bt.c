#include "ut/ut.h"
#include "ut/ut_win32.h"

static void* ut_bt_process;
static DWORD ut_bt_pid;

typedef ut_begin_packed struct symbol_info_s {
    SYMBOL_INFO info; char name[ut_bt_max_symbol];
} ut_end_packed symbol_info_t;

#pragma push_macro("ut_bt_load_dll")

#define ut_bt_load_dll(fn) do {              \
    if (GetModuleHandleA(fn) == null) {      \
        ut_fatal_win32err(LoadLibraryA(fn)); \
    }                                        \
} while (0)

static void ut_bt_init(void) {
    if (ut_bt_process == null) {
        ut_bt_load_dll("dbghelp.dll");
        ut_bt_load_dll("imagehlp.dll");
        DWORD options = SymGetOptions();
//      options |= SYMOPT_DEBUG;
        options |= SYMOPT_NO_PROMPTS;
        options |= SYMOPT_LOAD_LINES;
        options |= SYMOPT_UNDNAME;
        options |= SYMOPT_LOAD_ANYTHING;
        swear(SymSetOptions(options));
        ut_bt_pid = GetProcessId(GetCurrentProcess());
        swear(ut_bt_pid != 0);
        ut_bt_process = OpenProcess(PROCESS_ALL_ACCESS, false,
                                           ut_bt_pid);
        swear(ut_bt_process != null);
        swear(SymInitialize(ut_bt_process, null, true), "%s",
                            ut_str.error(ut_runtime.err()));
    }
}

#pragma pop_macro("ut_bt_load_dll")

static void ut_bt_capture(ut_bt_t* bt, int32_t skip) {
    ut_bt_init();
    SetLastError(0);
    bt->frames = CaptureStackBackTrace(1 + skip, ut_count_of(bt->stack),
        bt->stack, (DWORD*)&bt->hash);
    bt->error = ut_runtime.err();
}

static bool ut_bt_function(DWORD64 pc, SYMBOL_INFO* si) {
    // find DLL exported function
    bool found = false;
    const DWORD64 module_base = SymGetModuleBase64(ut_bt_process, pc);
    if (module_base != 0) {
        const DWORD flags = GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS;
        HMODULE module_handle = null;
        if (GetModuleHandleExA(flags, (const char*)pc, &module_handle)) {
            DWORD bytes = 0;
            IMAGE_EXPORT_DIRECTORY* dir = (IMAGE_EXPORT_DIRECTORY*)
                    ImageDirectoryEntryToDataEx(module_handle, true,
                            IMAGE_DIRECTORY_ENTRY_EXPORT, &bytes, null);
            if (dir) {
                uint8_t* m = (uint8_t*)module_handle;
                DWORD* functions = (DWORD*)(m + dir->AddressOfFunctions);
                DWORD* names = (DWORD*)(m + dir->AddressOfNames);
                WORD* ordinals = (WORD*)(m + dir->AddressOfNameOrdinals);
                DWORD64 address = 0; // closest address
                DWORD64 min_distance = (DWORD64)-1;
                const char* function = NULL; // closest function name
                for (DWORD i = 0; i < dir->NumberOfNames; i++) {
                    // function address
                    DWORD64 fa = (DWORD64)(m + functions[ordinals[i]]);
                    if (fa <= pc) {
                        DWORD64 distance = pc - fa;
                        if (distance < min_distance) {
                            min_distance = distance;
                            address = fa;
                            function = (const char*)(m + names[i]);
                        }
                    }
                }
                if (function != null) {
                    si->ModBase = (uint64_t)m;
                    snprintf(si->Name, si->MaxNameLen - 1, "%s", function);
                    si->Name[si->MaxNameLen - 1] = 0x00;
                    si->NameLen = (DWORD)strlen(si->Name);
                    si->Address = address;
                    found = true;
                }
            }
        }
    }
    return found;
}

// SimpleStackWalker::showVariablesAt() can be implemented if needed like this:
// https://accu.org/journals/overload/29/165/orr/
// https://github.com/rogerorr/articles/tree/main/Debugging_Optimised_Code
// https://github.com/rogerorr/articles/blob/main/Debugging_Optimised_Code/SimpleStackWalker.cpp#L301

static const void ut_bt_symbolize_inline_frame(ut_bt_t* bt,
        int32_t i, DWORD64 pc, DWORD inline_context, symbol_info_t* si) {
    si->info.Name[0] = 0;
    si->info.NameLen = 0;
    bt->file[i][0] = 0;
    bt->line[i] = 0;
    bt->symbol[i][0] = 0;
    DWORD64 displacement = 0;
    if (SymFromInlineContext(ut_bt_process, pc, inline_context,
                            &displacement, &si->info)) {
        ut_str_printf(bt->symbol[i], "%s", si->info.Name);
    } else {
        bt->error = ut_runtime.err();
    }
    IMAGEHLP_LINE64 li = { .SizeOfStruct = sizeof(IMAGEHLP_LINE64) };
    DWORD offset = 0;
    if (SymGetLineFromInlineContext(ut_bt_process,
                                    pc, inline_context, 0,
                                    &offset, &li)) {
        ut_str_printf(bt->file[i], "%s", li.FileName);
        bt->line[i] = li.LineNumber;
    }
}

// Too see kernel addresses in Stack Back Traces:
//
// Windows Registry Editor Version 5.00
// [HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Memory Management]
// "DisablePagingExecutive"=dword:00000001
//
// https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2003/cc757875(v=ws.10)

static int32_t ut_bt_symbolize_frame(ut_bt_t* bt, int32_t i) {
    const DWORD64 pc = (DWORD64)bt->stack[i];
    symbol_info_t si = {
        .info = { .SizeOfStruct = sizeof(SYMBOL_INFO),
                  .MaxNameLen = ut_count_of(si.name) }
    };
    bt->file[i][0] = 0;
    bt->line[i] = 0;
    bt->symbol[i][0] = 0;
    DWORD64 offsetFromSymbol = 0;
    const DWORD inline_count =
        SymAddrIncludeInlineTrace(ut_bt_process, pc);
    if (inline_count > 0) {
        DWORD ic = 0; // inline context
        DWORD fi = 0; // frame index
        if (SymQueryInlineTrace(ut_bt_process,
                                pc, 0, pc, pc, &ic, &fi)) {
            for (DWORD k = 0; k < inline_count; k++, ic++) {
                ut_bt_symbolize_inline_frame(bt, i, pc, ic, &si);
                i++;
            }
        }
    } else {
        if (SymFromAddr(ut_bt_process, pc, &offsetFromSymbol, &si.info)) {
            ut_str_printf(bt->symbol[i], "%s", si.info.Name);
            DWORD d = 0; // displacement
            IMAGEHLP_LINE64 ln = { .SizeOfStruct = sizeof(IMAGEHLP_LINE64) };
            if (SymGetLineFromAddr64(ut_bt_process, pc, &d, &ln)) {
                bt->line[i] = ln.LineNumber;
                ut_str_printf(bt->file[i], "%s", ln.FileName);
            } else {
                bt->error = ut_runtime.err();
                if (ut_bt_function(pc, &si.info)) {
                    GetModuleFileNameA((HANDLE)si.info.ModBase, bt->file[i],
                        ut_count_of(bt->file[i]) - 1);
                    bt->file[i][ut_count_of(bt->file[i]) - 1] = 0;
                    bt->line[i]    = 0;
                } else  {
                    bt->file[i][0] = 0x00;
                    bt->line[i]    = 0;
                }
            }
            i++;
        } else {
            bt->error = ut_runtime.err();
            if (ut_bt_function(pc, &si.info)) {
                ut_str_printf(bt->symbol[i], "%s", si.info.Name);
                GetModuleFileNameA((HANDLE)si.info.ModBase, bt->file[i],
                    ut_count_of(bt->file[i]) - 1);
                bt->file[i][ut_count_of(bt->file[i]) - 1] = 0;
                bt->error = 0;
                i++;
            } else {
                // will not do i++
            }
        }
    }
    return i;
}

static void ut_bt_symbolize_backtrace(ut_bt_t* bt) {
    assert(!bt->symbolized);
    bt->error = 0;
    ut_bt_init();
    // ut_bt_symbolize_frame() may produce zero, one or many frames
    int32_t n = bt->frames;
    void* stack[ut_countof(bt->stack)];
    memcpy(stack, bt->stack, n * sizeof(stack[0]));
    bt->frames = 0;
    for (int32_t i = 0; i < n && bt->frames < ut_count_of(bt->stack); i++) {
        bt->stack[bt->frames] = stack[i];
        bt->frames = ut_bt_symbolize_frame(bt, i);
    }
    bt->symbolized = true;
}

static void ut_bt_symbolize(ut_bt_t* bt) {
    if (!bt->symbolized) { ut_bt_symbolize_backtrace(bt); }
}

static const char* ut_bt_stops[] = {
    "main",
    "WinMain",
    "BaseThreadInitThunk",
    "RtlUserThreadStart",
    "mainCRTStartup",
    "WinMainCRTStartup",
    "invoke_main",
    "NdrInterfacePointerMemorySize",
    null
};

static void ut_bt_trace(const ut_bt_t* bt, const char* stop) {
    #pragma push_macro("ut_bt_glyph_called_from")
    #define ut_bt_glyph_called_from ut_glyph_north_west_arrow_with_hook
    assert(bt->symbolized, "need ut_bt.symbolize(bt)");
    const char** alt = stop != null && strcmp(stop, "*") == 0 ?
                       ut_bt_stops : null;
    for (int32_t i = 0; i < bt->frames; i++) {
        ut_debug.println(bt->file[i], bt->line[i], bt->symbol[i],
            ut_bt_glyph_called_from "%s",
            i == i < bt->frames - 1 ? "\n" : ""); // extra \n for last line
        if (stop != null && strcmp(bt->symbol[i], stop) == 0) { break; }
        const char** s = alt;
        while (s != null && *s != null && strcmp(bt->symbol[i], *s) != 0) { s++; }
        if (s != null && *s != null)  { break; }
    }
    #pragma pop_macro("ut_bt_glyph_called_from")
}


static const char* ut_bt_string(const ut_bt_t* bt,
        char* text, int32_t count) {
    assert(bt->symbolized, "need ut_bt.symbolize(bt)");
    char s[1024];
    char* p = text;
    int32_t n = count;
    for (int32_t i = 0; i < bt->frames && n > 128; i++) {
        int32_t line = bt->line[i];
        const char* file = bt->file[i];
        const char* name = bt->symbol[i];
        if (file[0] != 0 && name[0] != 0) {
            ut_str_printf(s, "%s(%d): %s\n", file, line, name);
        } else if (file[0] == 0 && name[0] != 0) {
            ut_str_printf(s, "%s\n", name);
        }
        s[ut_count_of(s) - 1] = 0;
        int32_t k = (int32_t)strlen(s);
        if (k < n) {
            memcpy(p, s, (size_t)k + 1);
            p += k;
            n -= k;
        }
    }
    return text;
}

typedef struct { char name[32]; } ut_bt_thread_name_t;

static ut_bt_thread_name_t ut_bt_thread_name(HANDLE thread) {
    ut_bt_thread_name_t tn;
    tn.name[0] = 0;
    wchar_t* thread_name = null;
    if (SUCCEEDED(GetThreadDescription(thread, &thread_name))) {
        ut_str.utf16to8(tn.name, ut_count_of(tn.name), thread_name);
        LocalFree(thread_name);
    }
    return tn;
}

static void ut_bt_context(ut_thread_t thread, const void* ctx,
        ut_bt_t* bt) {
    CONTEXT* context = (CONTEXT*)ctx;
    STACKFRAME64 stack_frame = { 0 };
    int machine_type = IMAGE_FILE_MACHINE_UNKNOWN;
    #if defined(_M_IX86)
        #error "Unsupported platform"
    #elif defined(_M_ARM64)
        machine_type = IMAGE_FILE_MACHINE_ARM64;
        stack_frame = (STACKFRAME64){
            .AddrPC    = {.Offset = context->Pc, .Mode = AddrModeFlat},
            .AddrFrame = {.Offset = context->Fp, .Mode = AddrModeFlat},
            .AddrStack = {.Offset = context->Sp, .Mode = AddrModeFlat}
        };
    #elif defined(_M_X64)
        machine_type = IMAGE_FILE_MACHINE_AMD64;
        stack_frame = (STACKFRAME64){
            .AddrPC    = {.Offset = context->Rip, .Mode = AddrModeFlat},
            .AddrFrame = {.Offset = context->Rbp, .Mode = AddrModeFlat},
            .AddrStack = {.Offset = context->Rsp, .Mode = AddrModeFlat}
        };
    #elif defined(_M_IA64)
        int machine_type = IMAGE_FILE_MACHINE_IA64;
        stack_frame = (STACKFRAME64){
            .AddrPC     = {.Offset = context->StIIP, .Mode = AddrModeFlat},
            .AddrFrame  = {.Offset = context->IntSp, .Mode = AddrModeFlat},
            .AddrBStore = {.Offset = context->RsBSP, .Mode = AddrModeFlat},
            .AddrStack  = {.Offset = context->IntSp, .Mode = AddrModeFlat}
        }
    #elif defined(_M_ARM64)
        machine_type = IMAGE_FILE_MACHINE_ARM64;
        stack_frame = (STACKFRAME64){
            .AddrPC    = {.Offset = context->Pc, .Mode = AddrModeFlat},
            .AddrFrame = {.Offset = context->Fp, .Mode = AddrModeFlat},
            .AddrStack = {.Offset = context->Sp, .Mode = AddrModeFlat}
        };
    #else
        #error "Unsupported platform"
    #endif
    ut_bt_init();
    while (StackWalk64(machine_type, ut_bt_process,
            (HANDLE)thread, &stack_frame, context, null,
            SymFunctionTableAccess64, SymGetModuleBase64, null)) {
        DWORD64 pc = stack_frame.AddrPC.Offset;
        if (pc == 0) { break; }
        if (bt->frames < ut_count_of(bt->stack)) {
            bt->stack[bt->frames] = (void*)pc;
            bt->frames = ut_bt_symbolize_frame(bt, bt->frames);
        }
    }
    bt->symbolized = true;
}

static void ut_bt_thread(HANDLE thread, ut_bt_t* bt) {
    bt->frames = 0;
    // cannot suspend callers thread
    swear(ut_thread.id_of(thread) != ut_thread.id());
    if (SuspendThread(thread) != (DWORD)-1) {
        CONTEXT context = { .ContextFlags = CONTEXT_FULL };
        GetThreadContext(thread, &context);
        ut_bt.context(thread, &context, bt);
        if (ResumeThread(thread) == (DWORD)-1) {
            ut_traceln("ResumeThread() failed %s", ut_str.error(ut_runtime.err()));
            ExitProcess(0xBD);
        }
    }
}

static void ut_bt_trace_self(const char* stop) {
    ut_bt_t bt = {{0}};
    ut_bt.capture(&bt, 2);
    ut_bt.symbolize(&bt);
    ut_bt.trace(&bt, stop);
}

static void ut_bt_trace_all_but_self(void) {
    ut_bt_init();
    assert(ut_bt_process != null && ut_bt_pid != 0);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        ut_traceln("CreateToolhelp32Snapshot failed %s",
                ut_str.error(ut_runtime.err()));
    } else {
        THREADENTRY32 te = { .dwSize = sizeof(THREADENTRY32) };
        if (!Thread32First(snapshot, &te)) {
            ut_traceln("Thread32First failed %s", ut_str.error(ut_runtime.err()));
        } else {
            do {
                if (te.th32OwnerProcessID == ut_bt_pid) {
                    static const DWORD flags = THREAD_ALL_ACCESS |
                       THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT;
                    uint32_t tid = te.th32ThreadID;
                    if (tid != (uint32_t)ut_thread.id()) {
                        HANDLE thread = OpenThread(flags, false, tid);
                        if (thread != null) {
                            ut_bt_t bt = {0};
                            ut_bt_thread(thread, &bt);
                            ut_bt_thread_name_t tn = ut_bt_thread_name(thread);
                            ut_debug.println(">Thread", tid, tn.name,
                                "id 0x%08X (%d)", tid, tid);
                            if (bt.frames > 0) {
                                ut_bt.trace(&bt, "*");
                            }
                            ut_debug.println("<Thread", tid, tn.name, "");
                            ut_win32_close_handle(thread);
                        }
                    }
                }
            } while (Thread32Next(snapshot, &te));
        }
        ut_win32_close_handle(snapshot);
    }
}

#ifdef UT_TESTS

static bool (*ut_bt_debug_tee)(const char* s, int32_t count);

static char  ut_bt_test_output[16 * 1024];
static char* ut_bt_test_output_p;

static bool ut_bt_tee(const char* s, int32_t count) {
    if (count > 0 && s[count - 1] == 0) { // zero terminated
        int32_t k = (int32_t)(uintptr_t)(
            ut_bt_test_output_p - ut_bt_test_output);
        int32_t space = ut_count_of(ut_bt_test_output) - k;
        if (count < space) {
            memcpy(ut_bt_test_output_p, s, count);
            ut_bt_test_output_p += count - 1; // w/o 0x00
        }
    } else {
        ut_debug.breakpoint(); // incorrect output() cannot append
    }
    return true; // intercepted, do not do OutputDebugString()
}

static void ut_bt_test_thread(void* e) {
    ut_event.wait(*(ut_event_t*)e);
}

static void ut_bt_test(void) {
    ut_bt_debug_tee = ut_debug.tee;
    ut_bt_test_output_p = ut_bt_test_output;
    ut_bt_test_output[0] = 0x00;
    ut_debug.tee = ut_bt_tee;
    ut_bt_t bt = {{0}};
    ut_bt.capture(&bt, 0);
    // ut_bt_test <- ut_runtime_test <- run <- main
    swear(bt.frames >= 3);
    ut_bt.symbolize(&bt);
    ut_bt.trace(&bt, null);
    ut_bt.trace(&bt, "main");
    ut_bt.trace(&bt, null);
    ut_bt.trace(&bt, "main");
    ut_event_t e = ut_event.create();
    ut_thread_t thread = ut_thread.start(ut_bt_test_thread, &e);
    ut_bt.trace_all_but_self();
    ut_event.set(e);
    ut_thread.join(thread, -1.0);
    ut_event.dispose(e);
    ut_debug.tee = ut_bt_debug_tee;
    if (ut_debug.verbosity.level >= ut_debug.verbosity.trace) {
        ut_debug.output(ut_bt_test_output,
            (int32_t)strlen(ut_bt_test_output) + 1);
    }
    swear(strstr(ut_bt_test_output, "ut_bt_test") != null,
          "%s", ut_bt_test_output);
    if (ut_debug.verbosity.level > ut_debug.verbosity.quiet) { ut_traceln("done"); }
}

#else

static void ut_bt_test(void) { }

#endif

ut_bt_if ut_bt = {
    .capture            = ut_bt_capture,
    .context            = ut_bt_context,
    .symbolize          = ut_bt_symbolize,
    .trace              = ut_bt_trace,
    .trace_self         = ut_bt_trace_self,
    .trace_all_but_self = ut_bt_trace_all_but_self,
    .string             = ut_bt_string,
    .test               = ut_bt_test
};
