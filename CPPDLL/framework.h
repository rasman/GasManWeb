#pragma once

#if defined(_WIN32) || defined(_WIN64)
// Windows-specific
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#include <unistd.h> // macOS-specific
#endif

#elif defined(__linux__)
#include <unistd.h> // Linux-specific

#else
#error "Unsupported platform"
#endif
