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
#define WM_ENABLE_STARTSTOP WM_USER + 0x00ff
#define WM_U2M_NOTIF_ICON WM_USER + 0x003f
#define WM_STARTSTOP_CONTROL WM_USER + 0x000d

/* flags to use with the ConnectedUSBDevs() function */
#define FILL_USB_LISTVIEW 10
#define IS_USB_CONNECTED 20

/* wrappers for win32 native memory management */
#define free(x) HeapFree(GetProcessHeap(), 0, x)
//this one is technically a wrapper for calloc since stdlib malloc doesn't 0 out the allocated bytes
#define malloc(x) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, x)
#define realloc(NULL, x) malloc(x)
#define calloc(x, y) malloc(x * y)

/* macro for LoadString which loads the error message 'y' from the currently
 * loaded locale DLL and puts it in the TCHAR string 'x' */
#define LoadLocaleErrMsg(x, y) LoadString(*g_hInst, ERR_ID(y), x, sizeof(x)/sizeof(x[0]));


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

extern BOOL onoff;
extern HANDLE u2m_StartStop_event;

extern HANDLE u2mMainThread;

extern HINSTANCE *g_hInst;

extern ULONG scanned_usb_ids[MAX_CONNECTED_USB][2];

extern char *cfg_filename; //the filename of the configuration file

extern WORD currentLangID; //the ID of the current language used


static void __MsgBoxGetLastError(LPTSTR lpszFunction)
{ 
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    INT err = GetLastError();
    TCHAR error_localized[255];

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, err, currentLangID, (LPTSTR)&lpMsgBuf, 0, NULL);

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
                   (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));

    StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                    _T("%s EC %lu: %s"),
                    lpszFunction, err, lpMsgBuf);

    err = LoadLocaleErrMsg(error_localized, 0);
    MessageBoxEx(NULL, (LPCTSTR)lpDisplayBuf, (err) ? error_localized : _T("Error!"), MB_OK, currentLangID); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}



BOOL InitU2MThread(user_input_data user_dat, HWND hwnd);
BOOL GetConnectedUSBDevs(HWND hDlg, ULONG VendorID, ULONG ProductID, USHORT flag);
VOID AddDeviceToUSBListView(HWND hDlg, TCHAR *dev_str, TCHAR *ven_str);
VOID InitU2MLogging(VOID);
VOID FreeModuleHeap(VOID);

BOOL parseConfFile(user_input_data *user_dat);
BOOL saveConfFile(user_input_data user_dat);
BOOL WriteDataToU2MReg(user_input_data user_dat);
BOOL GetU2MRegData(user_input_data *user_dat);
