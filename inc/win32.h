#pragma once
#ifdef WIN32

// #pragma warning(error  : 4013) // ERROR: 'foo' undefined; assuming extern returning int
// #pragma warning(disable: 4191) // 'type cast': unsafe conversion from 'FARPROC'
// #pragma warning(disable: 4255) // no function prototype given: converting '()' to '(void)'
// #pragma warning(disable: 4820) // 'n' bytes padding added after data member
// #pragma warning(disable: 5045) // Compiler will insert Spectre mitigation for memory load
// #pragma warning(disable: 4710) // function not inlined
// #pragma warning(disable: 4711) // function selected for automatic inline expansion
// #pragma warning(disable: 5039) // potentially throwing extern "C" function
// #pragma warning(disable: 4668) // is not defined as a preprocessor macro, replacing with '0'
// #pragma warning(disable: 4514) // unreferenced inline function has been removed
// // local compiler version and github msbuild compiler differ. github falling behind:
// #pragma warning(disable: 4619) // #pragma warning: there is no warning number ...

#ifdef __cplusplus
// [[fallthrough]] annotation is ignored by compiler (bug) as of 2022-11-11
// #pragma warning(disable: 5262) // implicit fall-through occurs here; Use [[fallthrough]] when a break statement is intentionally omitted between cases
// #pragma warning(disable: 5264) // 'const' variable is not used
#endif

#include <Windows.h>

#if defined(_DEBUG) || defined(DEBUG) // Microsoft runtime debug heap
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h> // _malloca()
#endif

#endif // WIN32
