#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#define LL_EXPORT __declspec(dllexport)
#define APIENTRY __stdcall
#define BOOL bool
#define HMODULE void*
#define DWORD unsigned long
#define LPVOID void*
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#include <unistd.h>
#endif
#define LL_EXPORT
#define APIENTRY
#define BOOL bool
#define HMODULE void*
#define DWORD unsigned long
#define LPVOID void*
#elif defined(__linux__)
#include <unistd.h>
#define LL_EXPORT
#define APIENTRY
#define BOOL bool
#define HMODULE void*
#define DWORD unsigned long
#define LPVOID void*
#endif
