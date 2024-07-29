#pragma once
#ifdef WIN32

#pragma warning(push)
#pragma warning(disable: 4255) // no function prototype: '()' to '(void)'
#pragma warning(disable: 4459) // declaration of '...' hides global declaration

#pragma push_macro("UNICODE")
#define UNICODE // always because otherwise IME does not work

// ut:
#include <Windows.h>  // used by:
#include <Psapi.h>    // both rt_loader.c and rt_processes.c
#include <shellapi.h> // rt_processes.c
#include <winternl.h> // rt_processes.c
#include <initguid.h>     // for knownfolders
#include <KnownFolders.h> // rt_files.c
#include <AclAPI.h>       // rt_files.c
#include <ShlObj_core.h>  // rt_files.c
#include <Shlwapi.h>      // rt_files.c
// ui:
#include <commdlg.h>
#include <dbghelp.h>
#include <dwmapi.h>
#include <imm.h>
#include <ShellScalingApi.h>
#include <tlhelp32.h>
#include <VersionHelpers.h>
#include <windowsx.h>
#include <winnt.h>

#pragma pop_macro("UNICODE")

#pragma warning(pop)

#include <fcntl.h>

#define rt_export __declspec(dllexport)

// Win32 API BOOL -> errno_t translation

#define rt_b2e(call) ((errno_t)(call ? 0 : GetLastError()))

void rt_win32_close_handle(void* h);
/* translate ix to error */
errno_t rt_wait_ix2e(uint32_t r);


#endif // WIN32
