/******************************************
*               U2MCommon.h                *
*        George Koskeridis (C)2015         *
 ******************************************/

#pragma once

#ifndef _UNICODE
# define UNICODE
#endif

#ifndef UNICODE
# define UNICODE
#endif

/* compile for Win7 */
#ifndef WINVER
# define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0601
#endif
#include <SDKDDKVer.h> //API versioning

#define WIN32_LEAN_AND_MEAN //skip uneccessary header files
#define NOCRYPT

#include <windows.h>
#include <winnt.h> //lanugage macros
#include <tchar.h> //unicode
#include <winerror.h> //error messages

#define _str(x) #x
#define str(x) _str(x)

#define _ver_str(x,y,z) str(x)"."str(y)"."str(z)
#define ver_str(x,y,z) _ver_str(x,y,z)
#define ver_commas(x,y,z) x,y,z
#define ver_dots(x,y,z) x.y.z

/* options for Microsoft VC++ compiler */
#ifdef _MSC_VER
# pragma warning(disable : 4172)
# define COMPILER_VERSION_STR str(_MSC_FULL_VER)
# define COMPILER_NAME_STR str(MSVC)
# define snprintf _snprintf
# define strdup _strdup
# define ATTRIB_UNUSED
/* options for MinGW */
#elif defined(__MINGW32__)
# pragma GCC diagnostic ignored "-Wreturn-local-addr"
# ifdef __MINGW64_VERSION_STR //mingw-w64
#  define COMPILER_VERSION_STR __MINGW64_VERSION_STR
#  define COMPILER_NAME_STR str(GCC(MinGW-w64))
# else //mingw
#  define COMPILER_VERSION_STR ver_str(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#  define COMPILER_NAME_STR str(GCC(MinGW))
# endif
# define ATTRIB_UNUSED __attribute__((unused))
/* options for Intel C/C++ compiler */
#elif defined(__INTEL_COMPILER)
# define COMPILER_VERSION_STR str(__INTEL_COMPILER)
# define COMPILER_NAME_STR str(Intel C/C++)
# define ATTRIB_UNUSED
#endif

#ifdef _WIN64
# define WINARCH str(x64)
#elif _WIN32
# define WINARCH str(Win32)
#endif

/* versioning */
#define U2MWin32_MAJOR 3
#define U2MWin32_MINOR 2
#define U2MWin32_PATCH 1

#define U2MWin32_VERSION_STR ver_str(U2MWin32_MAJOR,U2MWin32_MINOR,U2MWin32_PATCH)
