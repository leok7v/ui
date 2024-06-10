#pragma once
#ifdef WIN32

#pragma warning(push)
#pragma warning(disable: 4255) // no function prototype: '()' to '(void)'
#pragma warning(disable: 4459) // declaration of '...' hides global declaration

// ut:
#include <Windows.h>  // used by:
#include <Psapi.h>    // both ut_loader.c and ut_processes.c
#include <shellapi.h> // ut_processes.c
#include <winternl.h> // ut_processes.c
#include <initguid.h>     // for knownfolders
#include <KnownFolders.h> // ut_files.c
#include <AclAPI.h>       // ut_files.c
#include <ShlObj_core.h>  // ut_files.c
#include <Shlwapi.h>      // ut_files.c
// ui:
#include <windowsx.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <ShellScalingApi.h>
#include <VersionHelpers.h>
#include <dbghelp.h>
#include <tlhelp32.h>
#include <winnt.h>

#pragma warning(pop)

#include <fcntl.h>

#define ut_export __declspec(dllexport)

// Win32 API BOOL -> errno_t translation

#define ut_b2e(call) ((errno_t)(call ? 0 : GetLastError()))


#endif // WIN32
