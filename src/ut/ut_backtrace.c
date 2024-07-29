#include "ut/ut.h"
#include "ut/ut_win32.h"

static void* rt_backtrace_process;
static DWORD rt_backtrace_pid;

typedef ut_begin_packed struct symbol_info_s {
    SYMBOL_INFO info; char name[rt_backtrace_max_symbol];
} ut_end_packed symbol_info_t;

#pragma push_macro("rt_backtrace_load_dll")

#define rt_backtrace_load_dll(fn) do {              \
    if (GetModuleHandleA(fn) == null) {      \
        ut_fatal_win32err(LoadLibraryA(fn)); \
    }                                        \
} while (0)

static void rt_backtrace_init(void) {
    if (rt_backtrace_process == null) {
        rt_backtrace_load_dll("dbghelp.dll");
        rt_backtrace_load_dll("imagehlp.dll");
        DWORD options = SymGetOptions();
//      options |= SYMOPT_DEBUG;
        options |= SYMOPT_NO_PROMPTS;
        options |= SYMOPT_LOAD_LINES;
        options |= SYMOPT_UNDNAME;
        options |= SYMOPT_LOAD_ANYTHING;
        rt_swear(SymSetOptions(options));
        rt_backtrace_pid = GetProcessId(GetCurrentProcess());
        rt_swear(rt_backtrace_pid != 0);
        rt_backtrace_process = OpenProcess(PROCESS_ALL_ACCESS, false,
                                           rt_backtrace_pid);
        rt_swear(rt_backtrace_process != null);
        rt_swear(SymInitialize(rt_backtrace_process, null, true), "%s",
                            ut_str.error(rt_core.err()));
    }
}

#pragma pop_macro("rt_backtrace_load_dll")

static void rt_backtrace_capture(rt_backtrace_t* bt, int32_t skip) {
    rt_backtrace_init();
    SetLastError(0);
    bt->frames = CaptureStackBackTrace(1 + skip, ut_countof(bt->stack),
        bt->stack, (DWORD*)&bt->hash);
    bt->error = rt_core.err();
}

static bool rt_backtrace_function(DWORD64 pc, SYMBOL_INFO* si) {
    // find DLL exported function
    bool found = false;
    const DWORD64 module_base = SymGetModuleBase64(rt_backtrace_process, pc);
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

static const void rt_backtrace_symbolize_inline_frame(rt_backtrace_t* bt,
        int32_t i, DWORD64 pc, DWORD inline_context, symbol_info_t* si) {
    si->info.Name[0] = 0;
    si->info.NameLen = 0;
    bt->file[i][0] = 0;
    bt->line[i] = 0;
    bt->symbol[i][0] = 0;
    DWORD64 displacement = 0;
    if (SymFromInlineContext(rt_backtrace_process, pc, inline_context,
                            &displacement, &si->info)) {
        ut_str_printf(bt->symbol[i], "%s", si->info.Name);
    } else {
        bt->error = rt_core.err();
    }
    IMAGEHLP_LINE64 li = { .SizeOfStruct = sizeof(IMAGEHLP_LINE64) };
    DWORD offset = 0;
    if (SymGetLineFromInlineContext(rt_backtrace_process,
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

static int32_t rt_backtrace_symbolize_frame(rt_backtrace_t* bt, int32_t i) {
    const DWORD64 pc = (DWORD64)bt->stack[i];
    symbol_info_t si = {
        .info = { .SizeOfStruct = sizeof(SYMBOL_INFO),
                  .MaxNameLen = ut_countof(si.name) }
    };
    bt->file[i][0] = 0;
    bt->line[i] = 0;
    bt->symbol[i][0] = 0;
    DWORD64 offsetFromSymbol = 0;
    const DWORD inline_count =
        SymAddrIncludeInlineTrace(rt_backtrace_process, pc);
    if (inline_count > 0) {
        DWORD ic = 0; // inline context
        DWORD fi = 0; // frame index
        if (SymQueryInlineTrace(rt_backtrace_process,
                                pc, 0, pc, pc, &ic, &fi)) {
            for (DWORD k = 0; k < inline_count; k++, ic++) {
                rt_backtrace_symbolize_inline_frame(bt, i, pc, ic, &si);
                i++;
            }
        }
    } else {
        if (SymFromAddr(rt_backtrace_process, pc, &offsetFromSymbol, &si.info)) {
            ut_str_printf(bt->symbol[i], "%s", si.info.Name);
            DWORD d = 0; // displacement
            IMAGEHLP_LINE64 ln = { .SizeOfStruct = sizeof(IMAGEHLP_LINE64) };
            if (SymGetLineFromAddr64(rt_backtrace_process, pc, &d, &ln)) {
                bt->line[i] = ln.LineNumber;
                ut_str_printf(bt->file[i], "%s", ln.FileName);
            } else {
                bt->error = rt_core.err();
                if (rt_backtrace_function(pc, &si.info)) {
                    GetModuleFileNameA((HANDLE)si.info.ModBase, bt->file[i],
                        ut_countof(bt->file[i]) - 1);
                    bt->file[i][ut_countof(bt->file[i]) - 1] = 0;
                    bt->line[i]    = 0;
                } else  {
                    bt->file[i][0] = 0x00;
                    bt->line[i]    = 0;
                }
            }
            i++;
        } else {
            bt->error = rt_core.err();
            if (rt_backtrace_function(pc, &si.info)) {
                ut_str_printf(bt->symbol[i], "%s", si.info.Name);
                GetModuleFileNameA((HANDLE)si.info.ModBase, bt->file[i],
                    ut_countof(bt->file[i]) - 1);
                bt->file[i][ut_countof(bt->file[i]) - 1] = 0;
                bt->error = 0;
                i++;
            } else {
                // will not do i++
            }
        }
    }
    return i;
}

static void rt_backtrace_symbolize_backtrace(rt_backtrace_t* bt) {
    ut_assert(!bt->symbolized);
    bt->error = 0;
    rt_backtrace_init();
    // rt_backtrace_symbolize_frame() may produce zero, one or many frames
    int32_t n = bt->frames;
    void* stack[ut_countof(bt->stack)];
    memcpy(stack, bt->stack, n * sizeof(stack[0]));
    bt->frames = 0;
    for (int32_t i = 0; i < n && bt->frames < ut_countof(bt->stack); i++) {
        bt->stack[bt->frames] = stack[i];
        bt->frames = rt_backtrace_symbolize_frame(bt, i);
    }
    bt->symbolized = true;
}

static void rt_backtrace_symbolize(rt_backtrace_t* bt) {
    if (!bt->symbolized) { rt_backtrace_symbolize_backtrace(bt); }
}

static const char* rt_backtrace_stops[] = {
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

static void rt_backtrace_trace(const rt_backtrace_t* bt, const char* stop) {
    #pragma push_macro("rt_backtrace_glyph_called_from")
    #define rt_backtrace_glyph_called_from rt_glyph_north_west_arrow_with_hook
    ut_assert(bt->symbolized, "need rt_backtrace.symbolize(bt)");
    const char** alt = stop != null && strcmp(stop, "*") == 0 ?
                       rt_backtrace_stops : null;
    for (int32_t i = 0; i < bt->frames; i++) {
        rt_debug.println(bt->file[i], bt->line[i], bt->symbol[i],
            rt_backtrace_glyph_called_from "%s",
            i == i < bt->frames - 1 ? "\n" : ""); // extra \n for last line
        if (stop != null && strcmp(bt->symbol[i], stop) == 0) { break; }
        const char** s = alt;
        while (s != null && *s != null && strcmp(bt->symbol[i], *s) != 0) { s++; }
        if (s != null && *s != null)  { break; }
    }
    #pragma pop_macro("rt_backtrace_glyph_called_from")
}


static const char* rt_backtrace_string(const rt_backtrace_t* bt,
        char* text, int32_t count) {
    ut_assert(bt->symbolized, "need rt_backtrace.symbolize(bt)");
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
        s[ut_countof(s) - 1] = 0;
        int32_t k = (int32_t)strlen(s);
        if (k < n) {
            memcpy(p, s, (size_t)k + 1);
            p += k;
            n -= k;
        }
    }
    return text;
}

typedef struct { char name[32]; } rt_backtrace_thread_name_t;

static rt_backtrace_thread_name_t rt_backtrace_thread_name(HANDLE thread) {
    rt_backtrace_thread_name_t tn;
    tn.name[0] = 0;
    wchar_t* thread_name = null;
    if (SUCCEEDED(GetThreadDescription(thread, &thread_name))) {
        ut_str.utf16to8(tn.name, ut_countof(tn.name), thread_name, -1);
        LocalFree(thread_name);
    }
    return tn;
}

static void rt_backtrace_context(ut_thread_t thread, const void* ctx,
        rt_backtrace_t* bt) {
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
    rt_backtrace_init();
    while (StackWalk64(machine_type, rt_backtrace_process,
            (HANDLE)thread, &stack_frame, context, null,
            SymFunctionTableAccess64, SymGetModuleBase64, null)) {
        DWORD64 pc = stack_frame.AddrPC.Offset;
        if (pc == 0) { break; }
        if (bt->frames < ut_countof(bt->stack)) {
            bt->stack[bt->frames] = (void*)pc;
            bt->frames = rt_backtrace_symbolize_frame(bt, bt->frames);
        }
    }
    bt->symbolized = true;
}

static void rt_backtrace_thread(HANDLE thread, rt_backtrace_t* bt) {
    bt->frames = 0;
    // cannot suspend callers thread
    rt_swear(ut_thread.id_of(thread) != ut_thread.id());
    if (SuspendThread(thread) != (DWORD)-1) {
        CONTEXT context = { .ContextFlags = CONTEXT_FULL };
        GetThreadContext(thread, &context);
        rt_backtrace.context(thread, &context, bt);
        if (ResumeThread(thread) == (DWORD)-1) {
            ut_println("ResumeThread() failed %s", ut_str.error(rt_core.err()));
            ExitProcess(0xBD);
        }
    }
}

static void rt_backtrace_trace_self(const char* stop) {
    rt_backtrace_t bt = {{0}};
    rt_backtrace.capture(&bt, 2);
    rt_backtrace.symbolize(&bt);
    rt_backtrace.trace(&bt, stop);
}

static void rt_backtrace_trace_all_but_self(void) {
    rt_backtrace_init();
    ut_assert(rt_backtrace_process != null && rt_backtrace_pid != 0);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        ut_println("CreateToolhelp32Snapshot failed %s",
                ut_str.error(rt_core.err()));
    } else {
        THREADENTRY32 te = { .dwSize = sizeof(THREADENTRY32) };
        if (!Thread32First(snapshot, &te)) {
            ut_println("Thread32First failed %s", ut_str.error(rt_core.err()));
        } else {
            do {
                if (te.th32OwnerProcessID == rt_backtrace_pid) {
                    static const DWORD flags = THREAD_ALL_ACCESS |
                       THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT;
                    uint32_t tid = te.th32ThreadID;
                    if (tid != (uint32_t)ut_thread.id()) {
                        HANDLE thread = OpenThread(flags, false, tid);
                        if (thread != null) {
                            rt_backtrace_t bt = {0};
                            rt_backtrace_thread(thread, &bt);
                            rt_backtrace_thread_name_t tn = rt_backtrace_thread_name(thread);
                            rt_debug.println(">Thread", tid, tn.name,
                                "id 0x%08X (%d)", tid, tid);
                            if (bt.frames > 0) {
                                rt_backtrace.trace(&bt, "*");
                            }
                            rt_debug.println("<Thread", tid, tn.name, "");
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

static bool (*rt_backtrace_debug_tee)(const char* s, int32_t count);

static char  rt_backtrace_test_output[16 * 1024];
static char* rt_backtrace_test_output_p;

static bool rt_backtrace_tee(const char* s, int32_t count) {
    if (count > 0 && s[count - 1] == 0) { // zero terminated
        int32_t k = (int32_t)(uintptr_t)(
            rt_backtrace_test_output_p - rt_backtrace_test_output);
        int32_t space = ut_countof(rt_backtrace_test_output) - k;
        if (count < space) {
            memcpy(rt_backtrace_test_output_p, s, count);
            rt_backtrace_test_output_p += count - 1; // w/o 0x00
        }
    } else {
        rt_debug.breakpoint(); // incorrect output() cannot append
    }
    return true; // intercepted, do not do OutputDebugString()
}

static void rt_backtrace_test_thread(void* e) {
    ut_event.wait(*(ut_event_t*)e);
}

static void rt_backtrace_test(void) {
    rt_backtrace_debug_tee = rt_debug.tee;
    rt_backtrace_test_output_p = rt_backtrace_test_output;
    rt_backtrace_test_output[0] = 0x00;
    rt_debug.tee = rt_backtrace_tee;
    rt_backtrace_t bt = {{0}};
    rt_backtrace.capture(&bt, 0);
    // rt_backtrace_test <- rt_core_test <- run <- main
    rt_swear(bt.frames >= 3);
    rt_backtrace.symbolize(&bt);
    rt_backtrace.trace(&bt, null);
    rt_backtrace.trace(&bt, "main");
    rt_backtrace.trace(&bt, null);
    rt_backtrace.trace(&bt, "main");
    ut_event_t e = ut_event.create();
    ut_thread_t thread = ut_thread.start(rt_backtrace_test_thread, &e);
    rt_backtrace.trace_all_but_self();
    ut_event.set(e);
    ut_thread.join(thread, -1.0);
    ut_event.dispose(e);
    rt_debug.tee = rt_backtrace_debug_tee;
    if (rt_debug.verbosity.level >= rt_debug.verbosity.trace) {
        rt_debug.output(rt_backtrace_test_output,
            (int32_t)strlen(rt_backtrace_test_output) + 1);
    }
    rt_swear(strstr(rt_backtrace_test_output, "rt_backtrace_test") != null,
          "%s", rt_backtrace_test_output);
    if (rt_debug.verbosity.level > rt_debug.verbosity.quiet) { ut_println("done"); }
}

#else

static void rt_backtrace_test(void) { }

#endif

rt_backtrace_if rt_backtrace = {
    .capture            = rt_backtrace_capture,
    .context            = rt_backtrace_context,
    .symbolize          = rt_backtrace_symbolize,
    .trace              = rt_backtrace_trace,
    .trace_self         = rt_backtrace_trace_self,
    .trace_all_but_self = rt_backtrace_trace_all_but_self,
    .string             = rt_backtrace_string,
    .test               = rt_backtrace_test
};
