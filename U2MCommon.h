/******************************************
*               U2MCommon.h                *
*        George Koskeridis (C)2015         *
 ******************************************/

#pragma once

/* compile for Win7 */
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#include <SDKDDKVer.h> //API versioning


#include <windows.h>
#include <tchar.h> //unicode

#define _str(x) #x
#define str(x) _str(x)
#define _ver_str(x,y,z) str(x)"."str(y)"."str(z)
#define ver_str(x,y,z) _ver_str(x,y,z)

/* options for Microsoft VC++ compiler */
#ifdef _MSC_VER
# define COMPILER_VERSION_STR str(_MSC_FULL_VER)
# define COMPILER_NAME_STR str(MSVC)
# define snprintf _snprintf
# define strdup _strdup
# define ATTRIB_UNUSED
/* options for MinGW/GCC */
#elif defined(__MINGW32__)
# define COMPILER_VERSION_STR ver_str(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
# define COMPILER_NAME_STR str(GCC(MinGW))
# define ATTRIB_UNUSED __attribute__((unused))
/* options for Intel C/C++ compiler */
#elif defined(__INTEL_COMPILER)
# define COMPILER_VERSION_STR str(__INTEL_COMPILER)
# define COMPILER_NAME_STR str(Intel C/C++)
# define ATTRIB_UNUSED
#endif

#ifdef _WIN64
#define WINARCH str(x64)
#elif _WIN32
#define WINARCH str(Win32)
#endif

/* versioning */
#define U2MWin32_MAJOR 2
#define U2MWin32_MINOR 0
#define U2MWin32_PATCH 1

#define U2MWin32_VERSION_STR ver_str(U2MWin32_MAJOR,U2MWin32_MINOR,U2MWin32_PATCH)
