#pragma once
// ui is Windows-only. This header pulls in the Win32 SDK subset the ui.*
// implementation and ui consumers need. The runtime (posix.*) stays free of
// <Windows.h>; posix_b2e()/posix_win32_close_handle()/posix_wait_ix2e() are
// declared in posix.h (gated on _WIN32) and only reference Win32 APIs at the
// expansion site, so this header must be included before they are used.

#if defined(_WIN32)

#pragma warning(push)
#pragma warning(disable: 4255) // no function prototype: '()' to '(void)'
#pragma warning(disable: 4459) // declaration of '...' hides global declaration
#pragma warning(disable: 4668) // SDK headers (e.g. shellapi.h) test version
                               // macros like NTDDI_WIN10_GE that older SDKs
                               // do not define; harmless under /Wall

#pragma push_macro("UNICODE")
#define UNICODE // always because otherwise IME does not work

#include <Windows.h>
#include <Psapi.h>
#include <shellapi.h>
#include <winternl.h>
#include <initguid.h>
#include <KnownFolders.h>
#include <AclAPI.h>
#include <ShlObj_core.h>
#include <Shlwapi.h>
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

#endif // _WIN32
