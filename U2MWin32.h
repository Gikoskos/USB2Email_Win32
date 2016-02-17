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
#define MAX_CONNECTED_USB 100

#define ClearGlobalEmailData()                                         \
while (1) {                                                            \
    if (FROM) free(FROM);                                              \
    if (TO) free(TO);                                                  \
    if (CC) free(CC);                                                  \
    if (SUBJECT) free(SUBJECT);                                        \
    if (BODY) free(BODY);                                              \
    if (SMTP_SERVER) free(SMTP_SERVER);                                \
    if (PORT_STR) free(PORT_STR);                                      \
    if (SMTP_STR) free(SMTP_STR);                                      \
    SMTP_STR = PORT_STR = SMTP_SERVER = NULL;                          \
    FROM = TO = CC = SUBJECT = BODY = NULL;                            \
    break;                                                             \
}

extern char *pass, *FROM, *TO, *CC, *SUBJECT, *BODY, *SMTP_SERVER, *USBdev, *PORT_STR, *SMTP_STR;
extern BOOL ValidEmailCheck;
extern BOOL USBRefresh;
extern HANDLE u2mMainThread;
extern UINT TIMEOUT;
extern BOOL RUNNING;
extern UINT EMAIL_PAUSE;
extern UINT onoff;
extern UINT PORT;
extern UINT scanned_usb_ids[MAX_CONNECTED_USB][2];

BOOL InitU2MThread();
VOID fillUSBlist(HWND hwnd);
VOID AddDeviceToUSBListView(HWND hDlg, char *dev_str, char *ven_str);

BOOL parseConfFile();
