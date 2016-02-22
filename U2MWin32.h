/******************************************
*                U2MWin32.h                *
*        George Koskeridis (C)2015         *
 ******************************************/

#pragma once

#include "U2MCommon.h"

#include <commctrl.h> //common controls
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include <process.h> //threads
#include <winnt.h> //lanugage macros

#include "resources/resource.h"

#define MAX_CONNECTED_USB 20

#define WM_ENABLE_STARTSTOP WM_USER + 0x00dd

/* flags to use with the ConnectedUSBDevs() function */
#define FILL_USB_LISTVIEW 10
#define IS_USB_CONNECTED 20


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
extern UINT MAX_FAILED_EMAILS;
extern UINT onoff, PORT;
extern ULONG scanned_usb_ids[MAX_CONNECTED_USB][2];

UINT usb_id_selection[2];

extern char *cfg_filename;

BOOL InitU2MThread(HWND hwnd);
BOOL GetConnectedUSBDevs(HWND hDlg, USHORT flag);
VOID AddDeviceToUSBListView(HWND hDlg, char *dev_str, char *ven_str);

BOOL parseConfFile(VOID);
BOOL saveConfFile(VOID);
