/******************************************
*                U2MWin32.h                *
*        George Koskeridis (C)2015         *
 ******************************************/

#pragma once

#include "U2MCommon.h"

#include <commctrl.h> //common controls
#include <windowsx.h>
#include <signal.h>
#include <process.h> //threads
#include <objbase.h> //for CoCreateGuid()
#include <shellapi.h>
#include <sddl.h> //for security descriptor definition language strings and functions

#undef __CRT__NO_INLINE
#include <strsafe.h> //win32 native string handling


#include "resources/resource.h"
#define ERR_ID(x) ID_ERR_MSG_##x

#define MAX_CONNECTED_USB 20

/* maximum size for a U2M log file is 80 kibibytes */
#define MAX_LOG_FILE_SZ 81920

/* custom messages for the main window procedure */
#define WM_ENABLE_STARTSTOP WM_USER + 0x00dd
#define WM_U2M_NOTIF_ICON WM_USER + 0x000e
#define WM_RESET_MAINWINDOW_CONTROLS WM_USER + 0x000f

/* flags to use with the ConnectedUSBDevs() function */
#define FILL_USB_LISTVIEW 10
#define IS_USB_CONNECTED 20

/* wrappers for win32 native memory management */
#define free(x) HeapFree(GetProcessHeap(), 0, x)
//this one isn't actually a wrapper for stdlib malloc since malloc doesn't 0 out the allocated bytes
#define malloc(x) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, x)
#define realloc(NULL, x) malloc(x)
#define calloc(x, y) malloc(x * y)



/********************************************
*Macros to clear all data entered by the user*
 ********************************************/
#define ClearEmailData()                          \
do {                                              \
    if (user_dat.FROM) free(user_dat.FROM);       \
    if (user_dat.TO) free(user_dat.TO);           \
    if (user_dat.CC) free(user_dat.CC);           \
    if (user_dat.SUBJECT) free(user_dat.SUBJECT); \
    if (user_dat.BODY) free(user_dat.BODY);       \
    user_dat.FROM = user_dat.TO = user_dat.CC =   \
    user_dat.SUBJECT = user_dat.BODY = NULL;      \
} while (0)

#define ClearPwd()                          \
do {                                        \
    if (user_dat.pass) free(user_dat.pass); \
    user_dat.pass = NULL;                   \
} while (0)

#define ClearPrefs()                                      \
do {                                                      \
    if (user_dat.SMTP_SERVER) free(user_dat.SMTP_SERVER); \
    user_dat.SMTP_SERVER = NULL;                          \
    user_dat.PORT = 0;                                    \
    break;                                                \
} while (0)

#define DeleteScannedUSBIDs()                 \
do {                                          \
    int i, j;                                 \
    for (i = 0; i < MAX_CONNECTED_USB; i++) { \
        for (j = 0; j < 2; j++) {             \
            scanned_usb_ids[i][j] = 0;        \
        }                                     \
    }                                         \
} while (0)

#define DeleteU2MTrayIcon()                              \
do {                                                     \
    if (TrayIsInitialized) {                             \
        DestroyIcon(U2MTrayData.hIcon);                  \
        Shell_NotifyIcon(NIM_DELETE, &U2MTrayData);      \
    }                                                    \
    if (TrayIconMenu != NULL) DestroyMenu(TrayIconMenu); \
} while (0)

/* struct to store user data, like text field strings and check control booleans */
typedef struct user_input_data{
    char *pass;
    char *FROM;
    char *TO;
    char *CC;
    char *SUBJECT;
    char *BODY;
    char *SMTP_SERVER;
    UINT PORT;
    UINT TIMEOUT;
    UINT MAX_FAILED_EMAILS;
    BOOL TrayIcon;
    BOOL Autostart;
    BOOL USBRefresh;
    ULONG usb_id_selection[2];
    BOOL ValidEmailCheck;
} user_input_data;

/* enumeration of the locales, to be used as the index on the language DLL module array 
   last locale always has to be English */
enum locale_idx {
    GREEK_DLL = 0,
    /* other locales can be put here */
    ENGLISH_DLL
};


extern user_input_data user_dat;
extern BOOL onoff;
extern ULONG scanned_usb_ids[MAX_CONNECTED_USB][2];

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
    StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
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
