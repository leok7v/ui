#pragma once
#ifdef WIN32

#include <Windows.h>

#if (defined(_DEBUG) || defined(DEBUG)) && !defined(_malloca) // Microsoft runtime debug heap
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h> // _malloca()
#endif

#define export __declspec(dllexport)

#endif // WIN32
