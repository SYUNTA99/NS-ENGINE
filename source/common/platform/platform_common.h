// =============================================================================
// File: platform_common.h
// Description: Platform-specific defines (must be included before Windows.h)
// =============================================================================

#pragma once

// Windows version targeting (Windows 10+)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

// Reduce Windows.h bloat
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Prevent min/max macros from conflicting with std::min/std::max
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Enable strict type checking
#ifndef STRICT
#define STRICT
#endif

// Exclude rarely-used Windows components
#define NOSERVICE
#define NOMCX
#define NOIME
