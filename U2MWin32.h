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
#include <objbase.h> //for CoCreateGuid()
#include <shellapi.h>

#include "resources/resource.h"

#define MAX_CONNECTED_USB 20

#define WM_ENABLE_STARTSTOP WM_USER + 0x00dd
#define WM_U2M_NOTIF_ICON WM_USER + 0x000e

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

#define DeleteU2MTrayIcon()                                  \
while (1) {                                                  \
    if (TrayIsInitialized) {                                 \
        DestroyIcon(U2MTrayData.hIcon);                      \
        Shell_NotifyIcon(NIM_DELETE, &U2MTrayData);          \
    }                                                        \
    if (TrayIconMenu != NULL) DestroyMenu(TrayIconMenu);     \
    break;                                                   \
}

extern char *pass, *FROM, *TO, *CC, *SUBJECT, *BODY, *SMTP_SERVER, *USBdev;
extern BOOL ValidEmailCheck;
extern BOOL USBRefresh;
extern BOOL TrayIcon;
extern BOOL Autostart;
extern HANDLE u2mMainThread;
extern UINT TIMEOUT;
extern UINT MAX_FAILED_EMAILS;
extern BOOL onoff;
extern UINT PORT;
extern ULONG scanned_usb_ids[MAX_CONNECTED_USB][2];

extern ULONG usb_id_selection[2];

extern char *cfg_filename; //the filename of the configuration file

extern WORD currentLangID; //the ID of the current language used


#ifdef DEBUG
static inline void __MsgBoxGetLastError(LPTSTR lpszFunction) 
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, dw, currentLangID, (LPTSTR)&lpMsgBuf, 0, NULL);

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
                   (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    _sntprintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
               _T("%s EC %lu: %s"),
               lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, _T("Error!"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}
#endif


BOOL InitU2MThread(HWND hwnd);
BOOL GetConnectedUSBDevs(HWND hDlg, USHORT flag);
VOID AddDeviceToUSBListView(HWND hDlg, char *dev_str, char *ven_str);

BOOL parseConfFile(VOID);
BOOL saveConfFile(VOID);
BOOL WriteDataToU2MReg(VOID);
BOOL GetU2MRegData(VOID);
