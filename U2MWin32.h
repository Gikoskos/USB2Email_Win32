/******************************************
*                U2MWin32.h                *
*        George Koskeridis (C)2015         *
 ******************************************/

#pragma once

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <math.h>
#include <signal.h>
#include "resources/resource.h"

#define str(x) #x
#define _ver_str(x,y,z) str(x)"."str(y)"."str(z)
#define ver_str(x,y,z) _ver_str(x,y,z)

/* options for Microsoft VC++ compiler */
#ifdef _MSC_VER
# define COMPILER_VERSION_STR str(_MSC_FULL_VER)
# define COMPILER_NAME_STR str(Microsoft Visual C++)
# define snprintf _snprintf
# define strdup _strdup
# define __attribute__((unused))
/* options for GCC */
#elif defined(__GNUC__)
# define COMPILER_VERSION_STR ver_str(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
# define COMPILER_NAME_STR str(GCC)
/* options for Intel C/C++ compiler */
#elif defined(__INTEL_COMPILER)
# define COMPILER_VERSION_STR str(__INTEL_COMPILER)
# define COMPILER_NAME_STR str(Intel C/C++)
#endif

#define MAX_CONNECTED_USB 20


/* flags to use with the ConnectedUSBDevs() function */
#define FILL_USB_LISTVIEW 10
#define IS_USB_CONNECTED 20

/* versioning */
#define U2MWin32_MAJOR 1
#define U2MWin32_MINOR 3
#define U2MWin32_PATCH 0

#define U2MWin32_VERSION_STR ver_str(U2MWin32_MAJOR,U2MWin32_MINOR,U2MWin32_PATCH)

/********************************************
*Macros to clear all data entered by the user*
 ********************************************/
#define ClearEmailData()                         \
while (1) {                                      \
    if (FROM) free(FROM);                        \
    if (TO) free(TO);                            \
    if (CC) free(CC);                            \
    if (SUBJECT) free(SUBJECT);                  \
    if (BODY) free(BODY);                        \
    FROM = TO = CC = SUBJECT = BODY = NULL;      \
    break;                                       \
}

#define ClearPwd()                               \
while (1) {                                      \
    if (pass) free(pass);                        \
    pass = NULL;                                 \
    break;                                       \
}

#define ClearPrefs()                             \
while (1) {                                      \
    if (SMTP_SERVER) free(SMTP_SERVER);          \
    SMTP_SERVER = NULL;                          \
    PORT = 0;                                    \
    break;                                       \
}

#define DeleteScannedUSBIDs()                    \
while (1) {                                      \
    int i, j;                                    \
    for (i = 0; i < MAX_CONNECTED_USB; i++) {    \
        for (j = 0; j < 2; j++) {                \
            scanned_usb_ids[i][j] = 0;           \
        }                                        \
    }                                            \
    break;                                       \
}

extern char *pass, *FROM, *TO, *CC, *SUBJECT, *BODY, *SMTP_SERVER, *USBdev;
extern BOOL ValidEmailCheck;
extern BOOL USBRefresh;
extern HANDLE u2mMainThread;
extern UINT TIMEOUT;
extern UINT EMAIL_PAUSE;
extern UINT onoff, PORT, scanned_usb_ids[MAX_CONNECTED_USB][2];
UINT usb_id_selection[2];

BOOL InitU2MThread(HWND hwnd);
BOOL ConnectedUSBDevs(HWND hDlg, USHORT flag);
VOID AddDeviceToUSBListView(HWND hDlg, char *dev_str, char *ven_str);

BOOL parseConfFile(VOID);
BOOL saveConfFile(VOID);