
// GECOM platform detection macros

#ifdef _WIN32
#define GECOM_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#if defined(__APPLE__)
#define GECOM_PLATFORM_POSIX
#include "TargetConditionals.h"
#if TARGET_OS_MAC
#define GECOM_PLATFORM_MAC
#endif
#endif

#if defined(__unix__)
// TODO improve detection
#define GECOM_PLATFORM_POSIX
#endif
