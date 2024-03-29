#pragma once
#ifdef WIN32

#include <Windows.h>

#if defined(_DEBUG) || defined(DEBUG) // Microsoft runtime debug heap
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h> // _malloca()
#endif

#endif // WIN32
