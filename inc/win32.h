#pragma once
#ifdef WIN32

#include <Windows.h>  // used by:
#include <psapi.h>    // both loader.c and processes.c
#include <shellapi.h> // processes.c
#include <winternl.h> // processes.c
#include <initguid.h>     // for knownfolders
#include <knownfolders.h> // files.c
#include <aclapi.h>       // files.c
#include <shlobj_core.h>  // files.c
#include <shlwapi.h>      // files.c

#if (defined(_DEBUG) || defined(DEBUG)) && !defined(_malloca) // Microsoft runtime debug heap
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h> // _malloca()
#endif

#define export __declspec(dllexport)

#define b2e(call) (call ? 0 : GetLastError()) // BOOL -> errno_t

#endif // WIN32
