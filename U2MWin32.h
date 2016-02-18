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

#define ATTRIB(x) __attribute__((x))
#define MAX_CONNECTED_USB 20

#define IDC_CHOOSEUSBBUTTON     40027
#define IDC_EMAILBUTTON         40028
#define IDC_STARTSTOP           40029
#define IDC_TIMETRACK           40030

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
extern UINT EMAIL_PAUSE;
extern UINT onoff, PORT, scanned_usb_ids[MAX_CONNECTED_USB][2];
UINT usb_id_selection[2];

BOOL InitU2MThread(HWND hwnd);
BOOL ConnectedUSBDevs(HWND hDlg, USHORT flag);
VOID AddDeviceToUSBListView(HWND hDlg, char *dev_str, char *ven_str);

BOOL parseConfFile(VOID);
