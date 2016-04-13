/******************************************
*                U2MWin32.c                *
*        George Koskeridis (C)2015         *
 ******************************************/

#include "U2MWin32.h"


static const _TCHAR szClassName[] = TEXT("USB2Email");
HINSTANCE *g_hInst;

/* user input data from fields and controls all over the program */
static user_input_data user_dat = {
    .pass = NULL,
    .FROM = NULL,
    .TO = NULL,
    .CC = NULL,
    .SUBJECT = NULL,
    .BODY = NULL,
    .SMTP_SERVER = NULL,
    .PORT = 0,
    .TIMEOUT = 1000,
    .MAX_FAILED_EMAILS = 0,
    .TrayIcon = FALSE, //for the Tray Icon check control
    .Autostart = FALSE, //when TRUE the application autostarts on boot
    .USBRefresh = FALSE, //for the USB refresh check control
    .usb_id_selection = {0x0000, 0x0000},
    .ValidEmailCheck = FALSE //for the valid e-mail check control
};

BOOL USBdev_scan = FALSE; //helper global for the USB reload thread
BOOL HlpDlg_open = FALSE; //is TRUE when the Help dialog is open and FALSE when it's closed
BOOL TrayIsInitialized = FALSE; //saves memory by initializing the tray icon once at any point
BOOL AutostartWarning = TRUE; //when TRUE the warning dialog will be shown on autostart click

/*Is TRUE when the service is running and FALSE when it's not*/
BOOL onoff;

/*Main Window controls*/
HWND USBListButton, EMAILButton, STARTSTOP, time_track, ttrack_tooltip, ttrack_label;
TOOLINFO ttrack_struct;
HMENU MainMenu, TrayIconMenu;
NOTIFYICONDATA U2MTrayData = { 0 }; //tray area icon

static HWND hlp_hwnd = NULL; //handle to the Help window

HANDLE u2m_StartStop_event = NULL;

UINT usb_idx;

struct dll_data {
    TCHAR *filename;
    HMODULE module;
    HMENU locale_menu;
};

struct dll_data U2M_dlls[] = {
    {TEXT("U2MLocale_gr.dll"), NULL, NULL},
    /* other DLLs can be put here */
    {TEXT("U2MLocale_en.dll"), NULL, NULL}
};

WORD currentLangID;

/*Trackbar limits*/
#define T_MIN                     200
#define T_MAX                    2000

#define NO_SEPARATOR             '\0'

/* to be used with curl_global_init */
#define CURL_GLOBAL_SSL (1<<0)
#define CURL_GLOBAL_WIN32 (1<<1)

/***************************************************
* Prototypes for functions in this compilation unit *
 ***************************************************/
LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK HelpDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EmailDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK USBDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PwdDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PrefDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AboutDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


BOOL parseEmailDialogFields(HWND hwnd);
BOOL parsePrefDialogFields(HWND hwnd);
BOOL parsePwdField(HWND hwnd);
HWND WINAPI CreateBaloonToolTip(int toolID, HWND hDlg, TCHAR *pszText);
HWND WINAPI CreateTrackingToolTip(HWND hDlg, TCHAR *pszText);
BOOL isValidDomain(char *str, char SEPARATOR);
BOOL GetUSBListViewSelection(HWND hwnd);
TCHAR *GetLocaleStr(UINT iD);
HANDLE InitSingleInstanceMutex(VOID);
VOID ResetLocale(HWND hwnd);
HANDLE InitStartStopEvent(VOID);

VOID InitEmailDialog(HWND hwnd);
VOID InitAboutDialog(HWND hwnd);
VOID InitPasswordDialog(HWND hwnd);
VOID InitPreferencesDialog(HWND hwnd);
VOID InitHelpWindow(HWND hwnd);
VOID CenterChild(HWND hwnd);
VOID InitU2MTray(HWND hwnd);
BOOL InitU2MTrayMenu(VOID);
VOID SetU2MNotifyTip(VOID);
UINT PowerOf10(UINT x);
BOOL LoadLocaleDLLs(VOID);
VOID SetApplicationLanguage(VOID);
VOID DestroyLanguageLibraries(enum locale_idx UP_TO);

UINT CALLBACK RefreshUSBThread(LPVOID dat);
VOID AddDeviceToUSBListView(HWND hDlg, TCHAR *dev_str, TCHAR *ven_str);
VOID DeleteAll(VOID);
VOID GetFieldTextA(HWND hwnd, int nIDDlgItem, char **str);
VOID GetFieldText(HWND hwnd, int nIDDlgItem, TCHAR **str);
int MessageBoxLocalized(HWND hwnd, UINT text_id, UINT caption_id, UINT type);

/* Prototypes for functions in libcurl
 * only work with the MSVC build which links statically to all builds */
#if _MSC_VER
int curl_global_init(long flags);
void curl_global_cleanup(void);
#endif

UINT PowerOf10(UINT x)
{
    if (!x) return 1;

    UINT retvalue = 1;
    while (x--) {
        retvalue*=10;
    }

    return retvalue;
}

VOID DeleteAll(VOID)
{
    ClearEmailData();
    ClearPwd();
    ClearPrefs();
    DeleteU2MTrayIcon();
    FreeModuleHeap();
    DestroyLanguageLibraries(ENGLISH_DLL);
}

int MessageBoxLocalized(HWND hwnd, UINT text_id, UINT caption_id, UINT type)
{
    TCHAR tmp1[300], tmp2[300];

    LoadString(*g_hInst, text_id, tmp1, sizeof(tmp1)/sizeof(tmp1[0]));
    LoadString(*g_hInst, caption_id, tmp2, sizeof(tmp2)/sizeof(tmp2[0]));
    return MessageBoxEx(hwnd, tmp1, tmp2, type, currentLangID);
}

VOID InitPreferencesDialog(HWND hwnd)
{
    if (!DialogBoxParam(*g_hInst, MAKEINTRESOURCE(IDD_PREFDIALOG), hwnd, PrefDialogProcedure, (LPARAM)0)) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_52, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
    }
}

VOID InitPasswordDialog(HWND hwnd)
{
    if (!DialogBoxParam(*g_hInst, MAKEINTRESOURCE(IDD_PWDDIALOG), hwnd, PwdDialogProcedure, (LPARAM)0)) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_51, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
    }
}

VOID InitUSBDialog(HWND hwnd)
{
    if (!DialogBoxParam(*g_hInst, MAKEINTRESOURCE(IDD_USBDIALOG), hwnd, USBDialogProcedure, (LPARAM)0)) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_50, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
    }
}

VOID InitAboutDialog(HWND hwnd)
{
    if (!DialogBoxParam(*g_hInst, MAKEINTRESOURCE(IDD_ABOUTDIALOG), hwnd, AboutDialogProcedure, (LPARAM)0)) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_49, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
    }
}

VOID InitEmailDialog(HWND hwnd)
{
    if (!DialogBoxParam(*g_hInst, MAKEINTRESOURCE(IDD_EMAILDIALOG), hwnd, EmailDialogProcedure, (LPARAM)0)) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_48, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
    }
}

VOID InitHelpDialog(HWND hwnd)
{
    if (!HlpDlg_open) {
        HRSRC HelpDlgHrsrc = FindResource(*g_hInst, MAKEINTRESOURCE(IDD_HELPDIALOG), 
                                          MAKEINTRESOURCE(RT_DIALOG));
        HGLOBAL HelpDlgHandle = LoadResource(*g_hInst, HelpDlgHrsrc);
        LPCDLGTEMPLATE HelpDlgPtr = (LPCDLGTEMPLATE)LockResource(HelpDlgHandle);

        if (!(hlp_hwnd = CreateDialogIndirectParam(*g_hInst, HelpDlgPtr, hwnd, HelpDialogProcedure, (LPARAM)0))) {
            MessageBoxLocalized(hwnd, ID_ERR_MSG_47, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        }
    } else {
        //SetActiveWindow doesn't work well so i'm saving a global handle for the Help dialog
        //SetActiveWindow(GetDlgItem(hwnd, IDD_HELPDIALOG));
        SetActiveWindow(hlp_hwnd);
    }
}

BOOL isValidDomain(char *str, char SEPARATOR)
{
    if (strchr(str, ' '))
        return FALSE;

    char *c = strchr(str, '@') + 1;

    if (!c)
        return FALSE;
    if (c[0] == '.')
        return FALSE;
    if(strchr(c, '@'))
        return isValidDomain(strchr(c, '@'), SEPARATOR);

    char *d = c;
    c = strchr(c, '.');
    if (!c)
        return FALSE;
    if (c[0] == '\0' || d[1] == c[0])
        return FALSE;

    if (SEPARATOR != NO_SEPARATOR) {
        if (strchr(c, SEPARATOR))
            return isValidDomain(strchr(c, SEPARATOR), SEPARATOR);
    }
    return TRUE;
}

VOID GetFieldTextA(HWND hwnd, int nIDDlgItem, char **str)
{
    int bufsiz = GetWindowTextLengthA(GetDlgItem(hwnd, nIDDlgItem)) + 1;

    if (bufsiz > 1) {
        (*str) = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, bufsiz*1);
        GetDlgItemTextA(hwnd, nIDDlgItem, (*str), bufsiz);
    } else {
        if (*str)
            HeapFree(GetProcessHeap(), 0, *str);
        (*str) = NULL;
    }
}

VOID GetFieldText(HWND hwnd, int nIDDlgItem, TCHAR **str)
{
    int bufsiz = GetWindowTextLengthA(GetDlgItem(hwnd, nIDDlgItem)) + 1;

    if (bufsiz > 1) {
        (*str) = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, bufsiz*sizeof(TCHAR));
        GetDlgItemText(hwnd, nIDDlgItem, (*str), bufsiz);
    } else {
        if (*str)
            HeapFree(GetProcessHeap(), 0, *str);
        (*str) = NULL;
    }
}

HWND WINAPI CreateBaloonToolTip(int toolID, HWND hDlg, TCHAR *pszText)
{
    if (!toolID || !hDlg || !pszText) return NULL;

    HWND hwndTool = GetDlgItem(hDlg, toolID);

    HWND hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
                                  WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  hDlg, NULL,
                                  *g_hInst, NULL);

    if (!hwndTool || !hwndTip) return NULL;

    TOOLINFO toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo) - sizeof(void*); //<-fixes bug on win7+
    toolInfo.hwnd = hDlg;
    toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    toolInfo.uId = (UINT_PTR)hwndTool;
    toolInfo.lpszText = pszText;
    SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
    return hwndTip;
}

HWND WINAPI CreateTrackingToolTip(HWND hDlg, TCHAR *pszText)
{
    HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
                                 WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                 hDlg, NULL, *g_hInst, NULL);

    if (!hwndTT) return NULL;

    ttrack_struct.cbSize   = sizeof(TOOLINFO);
    ttrack_struct.uFlags   = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    ttrack_struct.hwnd     = hDlg;
    ttrack_struct.hinst    = *g_hInst;
    ttrack_struct.lpszText = pszText;
    ttrack_struct.uId      = (UINT_PTR)hDlg;

    GetClientRect(hDlg, &ttrack_struct.rect);

    SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ttrack_struct);
    return hwndTT;
}

BOOL parsePrefDialogFields(HWND hwnd)
{
    char *tmp1 = NULL, *tmp2 = NULL;
    size_t tmp1_len, tmp2_len;

    GetFieldTextA(hwnd, IDC_SERVERURLFIELD, &tmp1);
    if (!tmp1) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_46, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        return FALSE;
    }
    if (FAILED(StringCchLengthA(tmp1, MAX_BUFFER, &tmp1_len))) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_67, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    GetFieldTextA(hwnd, IDC_PORTFIELD, &tmp2);

    if (tmp2) {
        if (SUCCEEDED(StringCchLengthA(tmp2, MAX_BUFFER, &tmp2_len))) {
            if (tmp2_len > 5) {
                HeapFree(GetProcessHeap(), 0, tmp1);
                HeapFree(GetProcessHeap(), 0, tmp2);
                MessageBoxLocalized(hwnd, ID_ERR_MSG_44, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
                return FALSE;
            }

            user_dat.PORT = 0;
            UINT c;
            for (size_t i = 0; i < tmp2_len ; i++) {
                c = tmp2[i] - '0';
                user_dat.PORT = c*(PowerOf10(tmp2_len - (i + 1))) + user_dat.PORT;
            }
            HeapFree(GetProcessHeap(), 0, tmp2);

            if (user_dat.PORT > 65535) {
                HeapFree(GetProcessHeap(), 0, tmp1);
                MessageBoxLocalized(hwnd, ID_ERR_MSG_44, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
                return FALSE;
            }
        } else {
            MessageBoxLocalized(hwnd, ID_ERR_MSG_67, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        }
    } else {
        user_dat.PORT = 0;
        /*HeapFree(GetProcessHeap(), 0, tmp1);
        MessageBoxLocalized(hwnd, ID_ERR_MSG_45, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);*/
    }

    if ((user_dat.SMTP_SERVER = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, tmp1_len + 1)) != NULL) {
        StringCchPrintfA(user_dat.SMTP_SERVER, tmp1_len + 1, "%s", tmp1);
    } else {
        __MsgBoxGetLastError(hwnd, TEXT("HeapAlloc()"), __LINE__);
    }
    HeapFree(GetProcessHeap(), 0, tmp1);
    return TRUE;
}

BOOL parseEmailDialogFields(HWND hwnd)
{
    char *tmp = NULL;
    size_t tmp_len = 0;

    GetFieldTextA(hwnd, IDC_FROMFIELD, &tmp);
    if (!tmp) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_43, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if (user_dat.ValidEmailCheck) {
        if (!isValidDomain(tmp, NO_SEPARATOR)) {
            MessageBoxLocalized(hwnd, ID_ERR_MSG_42, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
            HeapFree(GetProcessHeap(), 0, tmp);
            return FALSE;
        }
    }
    if (SUCCEEDED(StringCchLengthA(tmp, MAX_BUFFER, &tmp_len))) {
        if ((user_dat.FROM = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, tmp_len + 1)) != NULL) {
            StringCchPrintfA(user_dat.FROM, tmp_len + 1, "%s", tmp);
        } else {
            __MsgBoxGetLastError(hwnd, TEXT("HeapAlloc()"), __LINE__);
        }
    } else {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_67, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
    }
    HeapFree(GetProcessHeap(), 0, tmp);
    tmp = NULL;
    tmp_len = 0;

    GetFieldTextA(hwnd, IDC_TOFIELD, &tmp);
    if (!tmp) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_41, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        return FALSE;
    }

    if (user_dat.ValidEmailCheck) {
        if (!isValidDomain(tmp, NO_SEPARATOR)) {
            MessageBoxLocalized(hwnd, ID_ERR_MSG_40, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
            HeapFree(GetProcessHeap(), 0, tmp);
            return FALSE;
        }
    }
    if (SUCCEEDED(StringCchLengthA(tmp, MAX_BUFFER, &tmp_len))) {
        if ((user_dat.TO = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, tmp_len + 1)) != NULL) {
            StringCchPrintfA(user_dat.TO, tmp_len + 1, "%s", tmp);
        } else {
            __MsgBoxGetLastError(hwnd, TEXT("HeapAlloc()"), __LINE__);
        }
    } else {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_67, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
    }
    HeapFree(GetProcessHeap(), 0, tmp);
    tmp = NULL;
    tmp_len = 0;

    GetFieldTextA(hwnd, IDC_CCFIELD, &tmp);

    if (!tmp) {
        user_dat.CC = NULL;
    } else {
        if (user_dat.ValidEmailCheck) {
            if (!isValidDomain(tmp, ';')) {
                HeapFree(GetProcessHeap(), 0, tmp);
                MessageBoxLocalized(hwnd, ID_ERR_MSG_39, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
                return FALSE;
            }
        }

        if (SUCCEEDED(StringCchLengthA(tmp, MAX_BUFFER, &tmp_len))) {
            if ((user_dat.CC = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, tmp_len + 1)) != NULL) {
                StringCchCopyNA(user_dat.CC, tmp_len + 1, tmp, tmp_len + 1);
            } else {
                __MsgBoxGetLastError(hwnd, TEXT("HeapAlloc()"), __LINE__);
            }
        } else {
            MessageBoxLocalized(hwnd, ID_ERR_MSG_67, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        }
        HeapFree(GetProcessHeap(), 0, tmp);
        tmp = NULL;
        tmp_len = 0;
    }

    GetFieldTextA(hwnd, IDC_SUBJECTFIELD, &tmp);
    if (!tmp) {
        if (MessageBoxLocalized(hwnd, ID_ERR_MSG_38, ID_ERR_MSG_37, MB_YESNO | MB_ICONASTERISK) == IDYES) {
            if ((user_dat.SUBJECT = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, 2)) != NULL) {
                StringCchPrintfA(user_dat.SUBJECT, 2, "");
            } else {
                __MsgBoxGetLastError(hwnd, TEXT("HeapAlloc()"), __LINE__);
            }
        } else {
            return FALSE;
        }
    } else {
        if (SUCCEEDED(StringCchLengthA(tmp, MAX_BUFFER, &tmp_len))) {
            if ((user_dat.SUBJECT = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, tmp_len + 1)) != NULL) {
                StringCchCopyNA(user_dat.SUBJECT, tmp_len + 1, tmp, tmp_len + 1);
            } else {
                __MsgBoxGetLastError(hwnd, TEXT("HeapAlloc()"), __LINE__);
            }
        } else {
            MessageBoxLocalized(hwnd, ID_ERR_MSG_67, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        }
        HeapFree(GetProcessHeap(), 0, tmp);
        tmp = NULL;
        tmp_len = 0;
    }

    GetFieldTextA(hwnd, IDC_MESSAGEFIELD, &tmp);
    if (!tmp) {
        if (MessageBoxLocalized(hwnd, ID_ERR_MSG_36, ID_ERR_MSG_35, MB_YESNO | MB_ICONASTERISK) == IDYES) {
            if ((user_dat.BODY = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, 2)) != NULL) {
                StringCchPrintfA(user_dat.BODY, 2, "");
            } else {
                __MsgBoxGetLastError(hwnd, TEXT("HeapAlloc()"), __LINE__);
            }
        } else {
            return FALSE;
        }
    } else {
        if (SUCCEEDED(StringCchLengthA(tmp, MAX_BUFFER, &tmp_len))) {
            if ((user_dat.BODY = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, tmp_len + 1)) != NULL) {
                StringCchCopyNA(user_dat.BODY, tmp_len + 1, tmp, tmp_len + 1);
            } else {
                __MsgBoxGetLastError(hwnd, TEXT("HeapAlloc()"), __LINE__);
            }
        } else {
            MessageBoxLocalized(hwnd, ID_ERR_MSG_67, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        }
        HeapFree(GetProcessHeap(), 0, tmp);
    }

    return TRUE;
}

BOOL parsePwdField(HWND hwnd)
{
    char *tmp = NULL;
    size_t tmp_len = 0;

    GetFieldTextA(hwnd, IDC_PWDFIELD, &tmp);
    if (!tmp) {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_34, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        return FALSE;
    } else {
        if (SUCCEEDED(StringCchLengthA(tmp, MAX_BUFFER, &tmp_len))) {
            if ((user_dat.pass = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, tmp_len + 1)) != NULL) {
                StringCchCopyNA(user_dat.pass, tmp_len + 1, tmp, tmp_len + 1);
            } else {
                __MsgBoxGetLastError(hwnd, TEXT("HeapAlloc()"), __LINE__);
            }
            HeapFree(GetProcessHeap(), 0, tmp);
        } else {
            MessageBoxLocalized(hwnd, ID_ERR_MSG_67, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
        }
    }
    return TRUE;
}

VOID AddDeviceToUSBListView(HWND hDlg, TCHAR *dev_str, TCHAR *ven_str)
{
    LVITEM dev;

    /* safety measure, theoretically unreachable */
    if (!dev_str) {
        TCHAR tmpmsg[255];
        LoadLocaleErrMsg(tmpmsg, 33);
        dev.pszText = tmpmsg;
    } else {
        dev.pszText = dev_str;
    }

    dev.mask = LVIF_TEXT | LVIF_STATE;
    dev.state = 0;
    dev.stateMask = 0;
    dev.iSubItem = 0;
    dev.iItem = usb_idx;
    ListView_InsertItem(GetDlgItem(hDlg, IDC_USBDEVLIST), &dev);
    if (!ven_str) {
        TCHAR tmpmsg[255];
        LoadLocaleErrMsg(tmpmsg, 32);
        ListView_SetItemText(GetDlgItem(hDlg, IDC_USBDEVLIST), usb_idx, 1, tmpmsg);
    } else {
        ListView_SetItemText(GetDlgItem(hDlg, IDC_USBDEVLIST), usb_idx, 1, ven_str);
    }
    usb_idx++;
}

BOOL GetUSBListViewSelection(HWND hwnd)
{
    INT usb_sel_idx = (INT)SendMessage(GetDlgItem(hwnd, IDC_USBDEVLIST),
                                       LVM_GETNEXTITEM, -1, LVNI_SELECTED);

    if (usb_sel_idx == -1) return FALSE;

    user_dat.usb_id_selection[0] = scanned_usb_ids[usb_sel_idx][0];
    user_dat.usb_id_selection[1] = scanned_usb_ids[usb_sel_idx][1];
    return TRUE;
}

VOID CenterChild(HWND hwnd)
{
    HWND hwndOwner = GetParent(hwnd);
    RECT rc, rcDlg, rcOwner;
    INT final_x, final_y;

    GetWindowRect(hwndOwner, &rcOwner);
    GetWindowRect(hwnd, &rcDlg);
    CopyRect(&rc, &rcOwner);
    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
    OffsetRect(&rc, -rc.left, -rc.top);
    OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

    final_x = rcOwner.left + (rc.right / 2);
    final_y = rcOwner.top + (rc.bottom / 2);
    if (final_x < 0) final_x = 0;
    if (final_y < 0) final_y = 0;
    SetWindowPos(hwnd, HWND_TOP, final_x, final_y, 0, 0, SWP_NOSIZE);
}

VOID SetApplicationLanguage(VOID)
{
    enum locale_idx curr_dll;

    switch (PRIMARYLANGID(currentLangID)) {
        case LANG_GREEK:
            curr_dll = GREEK_DLL;
            break;
        case LANG_ENGLISH:
        default:
            curr_dll = ENGLISH_DLL;
            break;
    }

    if (U2M_dlls[curr_dll].module != NULL && U2M_dlls[curr_dll].locale_menu != NULL) {
        *g_hInst = U2M_dlls[curr_dll].module;
        MainMenu = U2M_dlls[curr_dll].locale_menu;
        // this is for the default switch above; in case the language set is not supported
        // we default to English
        if (curr_dll == ENGLISH_DLL) currentLangID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
    }
}

VOID DestroyLanguageLibraries(enum locale_idx UP_TO)
{
    for (enum locale_idx i = GREEK_DLL; (i <= UP_TO) && (i <= ENGLISH_DLL); i++) {
        if (U2M_dlls[i].module) {
            FreeLibrary(U2M_dlls[i].module);
            U2M_dlls[i].module = NULL;
        }
        if (U2M_dlls[i].locale_menu) {
            DestroyMenu(U2M_dlls[i].locale_menu);
            U2M_dlls[i].locale_menu = NULL;
        }
    }
}

BOOL LoadLocaleDLLs(VOID)
{
    BOOL show_missing_libs_err = TRUE, show_missing_menu_err = TRUE;
    /* loading the language dlls */
    for (enum locale_idx i = GREEK_DLL; i <= ENGLISH_DLL; i++) {
        U2M_dlls[i].module = LoadLibraryEx(U2M_dlls[i].filename, NULL, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE);
        if (!U2M_dlls[i].module) {
            if (show_missing_libs_err) {
                if (MessageBox(NULL,
                               TEXT("A language pack failed to load. ")
                               TEXT("The application might malfunction. ")
                               TEXT("Are you sure you want to continue?"),
                               TEXT("Failed loading language package!"),
                               MB_ICONASTERISK | MB_YESNO) == IDNO) {
                    DestroyLanguageLibraries(i - 1); //free all the libraries up to this point
                    return FALSE;
                }
                show_missing_libs_err = FALSE;
            }
            U2M_dlls[i].locale_menu = NULL;
            continue;
        }
        //after the language pack is succesfully loaded we check to see if the Product Version of the DLL
        //and the version of USB2Email match (this could prevent possible DLL hijacking)
        LPBYTE dllver_inf;
        VS_FIXEDFILEINFO *dllver_inf_spec;
        UINT dllver_inf_spec_sz;
        DWORD dllver_sz = GetFileVersionInfoSize(U2M_dlls[i].filename, NULL); //get the size of the DLL
        if (!dllver_sz) {
            __MsgBoxGetLastError(NULL, TEXT("GetFileVersionInfoSize()"), __LINE__);
            FreeLibrary(U2M_dlls[i].module);
            U2M_dlls[i].module = NULL;
            continue;
        }
        dllver_inf = HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, sizeof(BYTE) * dllver_sz);
        if (!GetFileVersionInfo(U2M_dlls[i].filename, (DWORD)0, dllver_sz, (LPVOID)dllver_inf)) {
            __MsgBoxGetLastError(NULL, TEXT("GetFileVersionInfo()"), __LINE__);
            FreeLibrary(U2M_dlls[i].module);
            U2M_dlls[i].module = NULL;
            HeapFree(GetProcessHeap(), 0, dllver_inf);
            continue;
        }

        if (!VerQueryValue(dllver_inf, _T("\\"), (LPVOID *)&dllver_inf_spec, &dllver_inf_spec_sz)) {
            __MsgBoxGetLastError(NULL, TEXT("VerQueryValue()"), __LINE__);
            FreeLibrary(U2M_dlls[i].module);
            U2M_dlls[i].module = NULL;
            HeapFree(GetProcessHeap(), 0, dllver_inf);
            continue;
        }
        if (HIWORD(dllver_inf_spec->dwProductVersionMS) != U2MWin32_MAJOR ||
            LOWORD(dllver_inf_spec->dwProductVersionMS) != U2MWin32_MINOR ||
            HIWORD(dllver_inf_spec->dwProductVersionLS) != U2MWin32_PATCH) {
            TCHAR buf[255];

            HeapFree(GetProcessHeap(), 0, dllver_inf);
            StringCchPrintf(buf, 255, _T("%s%s%s%s%s"),
                            TEXT("The version of the language pack \'"),
                            U2M_dlls[i].filename,
                            TEXT("\' mismatches the version of USB2Email. "),
                            TEXT("The application might malfunction. "),
                            TEXT("Are you sure you want to continue?"));
            if (MessageBox(NULL, buf, TEXT("Failed loading language package!"), MB_ICONASTERISK | MB_YESNO) == IDNO) {
                DestroyLanguageLibraries(i - 1);
                return FALSE;
            }
            continue;
        } else {
            HeapFree(GetProcessHeap(), 0, dllver_inf);
        }

        U2M_dlls[i].locale_menu = LoadMenu(U2M_dlls[i].module, MAKEINTRESOURCE(IDR_MAINMENU));
        if (!U2M_dlls[i].locale_menu) {
            if (show_missing_menu_err) {
                if (MessageBox(NULL,
                               TEXT("One or more menu resources failed to load. ")
                               TEXT("The application might malfunction. ")
                               TEXT("Are you sure you want to continue?"),
                               TEXT("Failed loading menu resource!"),
                               MB_ICONASTERISK | MB_YESNO) == IDNO) {
                    DestroyLanguageLibraries(i - 1); //free all the libraries up to this point
                    return FALSE;
                }
                show_missing_menu_err = FALSE;
            }
            FreeLibrary(U2M_dlls[i].module);
            U2M_dlls[i].module = NULL;
            continue;
        }
        // the loop gets here only if both the library and the menu were loaded successfully
        // the program locale defaults to the last library that was loaded succesfully
        // this is a failsafe measure, in case the default language pack of the user's Windows isn't loaded
        *g_hInst = U2M_dlls[i].module;
        MainMenu = U2M_dlls[i].locale_menu;
    }

    currentLangID = GetUserDefaultUILanguage(); //get the language of the system
    SetApplicationLanguage();

    return TRUE;
}

VOID InitU2MTray(HWND hwnd)
{
    BOOL success = TRUE;

    if (!TrayIsInitialized) {
    //stupid bug in mingw-w64 toolchain doesn't link against high DPI LoadIconMetric function
#ifndef __MINGW32__ 
        if (LoadIconMetric(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USB2MAILICONSMALL),
                       LIM_SMALL, &(U2MTrayData.hIcon)) != S_OK) success = FALSE;
#else //RIP open-source compilers
        U2MTrayData.hIcon = LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDI_USB2MAILICONSMALL),
                                      IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                      GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
#endif
        if (!U2MTrayData.hIcon) success = FALSE;
        //note: GUID breaks compatibility, this only works on Win7 and up
        if (CoCreateGuid(&(U2MTrayData.guidItem)) != S_OK) success = FALSE;

        U2MTrayData.cbSize = sizeof(U2MTrayData); //Win Svista and later
        U2MTrayData.hWnd = hwnd;
        //U2MTrayData.dwState = U2MTrayData.dwStateMask = NIS_SHAREDICON;
        U2MTrayData.uCallbackMessage = WM_U2M_NOTIF_ICON;
        U2MTrayData.uVersion = NOTIFYICON_VERSION_4;
        U2MTrayData.uFlags = NIF_ICON | NIF_GUID | NIF_SHOWTIP | NIF_TIP | NIF_MESSAGE;
        TrayIsInitialized = TRUE;
    }

    if (success) {
        TCHAR tmpmsg1[50];
        LoadLocaleErrMsg(tmpmsg1, 58);
        StringCchCopy(U2MTrayData.szTip, 50, tmpmsg1);
        Shell_NotifyIcon(NIM_ADD, &U2MTrayData);
        Shell_NotifyIcon(NIM_SETVERSION, &U2MTrayData);
    } else {
        MessageBoxLocalized(hwnd, ID_ERR_MSG_30, ID_ERR_MSG_31, MB_OK | MB_ICONERROR);
    }
}

VOID SetU2MNotifyTip(VOID)
{
    if (TrayIsInitialized && user_dat.TrayIcon) {
        if (onoff) {
            TCHAR tmpmsg1[50];
            LoadLocaleErrMsg(tmpmsg1, 59);
            StringCchCopy(U2MTrayData.szTip, 50, tmpmsg1);
            Shell_NotifyIcon(NIM_MODIFY, &U2MTrayData);
        } else {
            TCHAR tmpmsg1[50];
            LoadLocaleErrMsg(tmpmsg1, 58);
            StringCchCopy(U2MTrayData.szTip, 50, tmpmsg1);
            Shell_NotifyIcon(NIM_MODIFY, &U2MTrayData);
        }
    }
}

VOID ResetLocale(HWND hwnd)
{
    TCHAR tmp1[255], tmp2[255], tmp3[255], tmp4[255];

    SetApplicationLanguage();

    LoadLocaleErrMsg(tmp1, 21);
    LoadLocaleErrMsg(tmp2, 19);
    if (onoff == TRUE) {
        LoadLocaleErrMsg(tmp3, 10);
    } else {
        LoadLocaleErrMsg(tmp3, 9);
    }
    LoadLocaleErrMsg(tmp4, 15);

    if (HlpDlg_open == TRUE) {
        TCHAR tmphlp1[7500], tmphlp2[2500], tmphlpcaption[255];

        LoadString(*g_hInst, ID_HELP_MSG1, tmphlp1, sizeof(tmphlp1) / sizeof(tmphlp1[0]));
        LoadString(*g_hInst, ID_HELP_MSG2, tmphlp2, sizeof(tmphlp2) / sizeof(tmphlp2[0]));
        LoadString(*g_hInst, HELP_DLG_CAPTION, tmphlpcaption, sizeof(tmphlpcaption) / sizeof(tmphlpcaption[0]));
        StringCchCat(tmphlp1, 7500, tmphlp2);
        SetDlgItemText(hlp_hwnd, IDC_HELP_TEXT, tmphlp1);
        SetWindowText(hlp_hwnd, tmphlpcaption);
    }

    SetWindowText(USBListButton, tmp1);
    SetWindowText(EMAILButton, tmp2);
    SetWindowText(STARTSTOP, tmp3);
    SetWindowText(ttrack_label, tmp4);

    SetMenu(hwnd, MainMenu);
    SetU2MNotifyTip();
    CheckMenuItem(GetMenu(hwnd), IDM_AUTOSTART,
                  (user_dat.Autostart)?(MF_BYCOMMAND | MF_CHECKED):(MF_BYCOMMAND | MF_UNCHECKED));
    DrawMenuBar(hwnd);
}

HANDLE InitSingleInstanceMutex(VOID)
{
    SECURITY_ATTRIBUTES mtx_sa;
    TCHAR *szSD = TEXT("D:(D;OICI;GA;;;BG)(D;OICI;GA;;;AN)(A;OICI;GRGWGX;;;AU)(A;OICI;GA;;;BA)");
    BOOL DACL_success;
    HANDLE retvalue;

    mtx_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    mtx_sa.bInheritHandle = FALSE;
    DACL_success = ConvertStringSecurityDescriptorToSecurityDescriptor(szSD,
                       SDDL_REVISION_1, &(mtx_sa.lpSecurityDescriptor), NULL);

    retvalue = CreateMutex((DACL_success) ? (&mtx_sa) : NULL, FALSE,
                           TEXT("Global\\{USB2Email-2CB5E714-8B05-4DFC-B8B0-4CBECEEC00E3}"));
    LocalFree(mtx_sa.lpSecurityDescriptor);

    return retvalue;
}

HANDLE InitStartStopEvent(VOID)
{
    GUID startstop_guid;
    TCHAR guid_name[255], *predefined_name = TEXT("U2MStartStopEvent");
    LPOLESTR startstop_guid_str = (LPOLESTR)NULL;

    //create a guid
    if (FAILED(CoCreateGuid(&startstop_guid))) {
        guid_name[0] = TEXT('\0');
        goto CREATE_EV;
    }
    //allocate memory for the OLE string to put the guid in
    startstop_guid_str = (LPOLESTR)CoTaskMemAlloc(100);
    if (startstop_guid_str != (LPOLESTR)NULL) {
        //convert guid to OLE string
        if (!StringFromGUID2(&startstop_guid, startstop_guid_str, 100)) {
            CoTaskMemFree(startstop_guid_str);
            guid_name[0] = TEXT('\0');
            goto CREATE_EV;
        }
        //write to the final event name string a unique name
        if (FAILED(StringCchPrintf(guid_name, 255, TEXT("%s-%S"), predefined_name, startstop_guid_str))) {
            CoTaskMemFree(startstop_guid_str);
            guid_name[0] = TEXT('\0');
            goto CREATE_EV;
        }
        CoTaskMemFree(startstop_guid_str);
    }

CREATE_EV:
    //we use the unique guid event name, if it was created succesfully, else we default to
    //a pre-defined name which increases possibility of conflict
    return CreateEvent(NULL, TRUE, FALSE, 
                       (guid_name[0] == TEXT('\0')) ? predefined_name : guid_name);
}

UINT CALLBACK RefreshUSBThread(LPVOID dat)
{
    HWND hwnd = (HWND)dat;

    if (hwnd == NULL) {
        _endthreadex(1);
        return 1;
    }

    while (WaitForSingleObject(u2m_StartStop_event, 1000) == WAIT_TIMEOUT) {
        SendMessageTimeout(hwnd, WM_COMMAND,
                           MAKEWPARAM((WORD)IDUSBREFRESH, 0),
                           (LPARAM)0, SMTO_NORMAL,
                           0, NULL);
    }
    _endthreadex(0);
    return 0;
}

INT_PTR CALLBACK AboutDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static BOOL LibVisible = FALSE;
    static HFONT DlgFont = NULL;
    static HWND lconf_link = NULL, lquick_link = NULL, lcurl_link = NULL;
    static HICON about_usb_icon = NULL;

    switch (msg) {
        case WM_INITDIALOG:
            about_usb_icon = (HICON)LoadImage(GetModuleHandle(NULL),
                                    MAKEINTRESOURCE(IDI_USB2MAILICONLARGE),
                                    IMAGE_ICON, 128, 128, 0);
            if (about_usb_icon)
                SendDlgItemMessage(hwnd, IDOK, BM_SETIMAGE,
                                  (WPARAM)IMAGE_ICON, (LPARAM)about_usb_icon);

            DlgFont = CreateFont(12, 0, 0, 0, 400,
                                 FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                                 CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                 VARIABLE_PITCH, TEXT("Ms Shell Dlg"));

            lquick_link = CreateWindowEx(0, WC_LINK, 
                                         TEXT("<a href=\"https://sourceforge.net/projects/libquickmail/\">libquickmail 0.1.21</a>")
                                         TEXT(" which depends on"),
                                         WS_VISIBLE | WS_CHILD | WS_TABSTOP | SS_LEFT,
                                         10, 152, 180, 14, hwnd, NULL, GetModuleHandle(NULL), NULL);
            if (lquick_link)
                SendMessage(lquick_link, WM_SETFONT, (WPARAM)DlgFont, (LPARAM)TRUE);

            lconf_link = CreateWindowEx(0, WC_LINK,
                                        TEXT("<a href=\"https://github.com/martinh/libconfuse/\">libconfuse 3.0</a>"),
                                        WS_VISIBLE | WS_CHILD | WS_TABSTOP | SS_LEFT,
                                        10, 166, 150, 14, hwnd, NULL, GetModuleHandle(NULL), NULL);
            if (lconf_link)
                SendMessage(lconf_link, WM_SETFONT, (WPARAM)DlgFont, (LPARAM)TRUE);

            lcurl_link = CreateWindowEx(0, WC_LINK, 
                                        TEXT("<a href=\"https://curl.haxx.se/libcurl/\">libcurl</a>")
                                        TEXT(" and ")
                                        TEXT("<a href=\"https://www.openssl.org/\">OpenSSL</a>"),
                                        WS_VISIBLE | WS_CHILD | WS_TABSTOP | SS_LEFT,
                                        190, 152, 150, 14, hwnd, NULL, GetModuleHandle(NULL), NULL);
            if (lcurl_link)
                SendMessage(lcurl_link, WM_SETFONT, (WPARAM)DlgFont, (LPARAM)TRUE);

            SetDlgItemTextA(hwnd, IDC_ABOUT_BUILD, "USB2Email " U2MWin32_VERSION_STR " " WINARCH);
			SetDlgItemTextA(hwnd, IDC_ABOUT_COMPILER, "built with " COMPILER_NAME_STR " " COMPILER_VERSION_STR);
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case NM_CLICK:
                case NM_RETURN:
                    {
                        PNMLINK pNMLink = (PNMLINK)lParam;
                        LITEM item = pNMLink->item;

                        if (((LPNMHDR)lParam)->hwndFrom == lquick_link ||
                            ((LPNMHDR)lParam)->hwndFrom == lconf_link ||
                            ((LPNMHDR)lParam)->hwndFrom == lcurl_link) {
                            ShellExecute(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
                        }
                    }
                    break;
            }
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    LibVisible = FALSE;
                    DeleteObject(DlgFont);
                    DestroyWindow(lquick_link);
                    DestroyWindow(lconf_link);
                    DestroyWindow(lcurl_link);
                    DestroyIcon(about_usb_icon);
                    EndDialog(hwnd, (INT_PTR)TRUE);
                    return (INT_PTR)TRUE;
                case IDUSB2MAILLIBS:
                    LibVisible = !LibVisible;
                    {
                        int i, width, height;
                        RECT DlgRect;

                        if (!GetWindowRect(hwnd, &DlgRect)) return (INT_PTR)TRUE;
                        width = DlgRect.right - DlgRect.left;
                        height = DlgRect.bottom - DlgRect.top;

                        for (i = 1; i <= 35; i++) {
                            HDWP winnum = BeginDeferWindowPos(1);
                            if (!winnum) continue;

                            Sleep(1);
                            if (LibVisible) {
                                DeferWindowPos(winnum, hwnd, HWND_TOP, 0, 0, width, height + i,
                                               SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
                            } else {
                                DeferWindowPos(winnum, hwnd, HWND_TOP, 0, 0, width, height - i,
                                               SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
                            }
                            EndDeferWindowPos(winnum);
                        }
                    }
                    return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK PwdDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
            if (user_dat.pass)
                SetDlgItemTextA(hwnd, IDC_PWDFIELD, user_dat.pass);
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    ClearPwd();
                    if (!parsePwdField(hwnd)) {
                        ClearPwd();
                        return (INT_PTR)TRUE;
                    }
                case IDCANCEL:
                    EndDialog(hwnd, (INT_PTR)TRUE);
                    return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK PrefDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
            if (user_dat.SMTP_SERVER)
                SetDlgItemTextA(hwnd, IDC_SERVERURLFIELD, user_dat.SMTP_SERVER);
            if (user_dat.PORT) {
                char PORT_STR[9];
                StringCchPrintfA(PORT_STR, 9, "%u", user_dat.PORT);
                SetDlgItemTextA(hwnd, IDC_PORTFIELD, PORT_STR);
            }
            CheckDlgButton(hwnd, IDC_CHECKVALIDEMAIL, (user_dat.ValidEmailCheck)?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECKUSBREFRESH, (user_dat.USBRefresh)?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECKMINTOTRAY, (user_dat.TrayIcon)?BST_CHECKED:BST_UNCHECKED);
            SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 50));
            SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)user_dat.MAX_FAILED_EMAILS);
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_SAVECONFBUTTON:
                    saveConfFile(&user_dat);
                    return (INT_PTR)TRUE;
                case IDAPPLY:
                case IDOK:
                    ClearPrefs();
                    if (parsePrefDialogFields(hwnd)) {
                        // i'm storing the check controls' values only if the 
                        // data entered on the input fields was valid
                        user_dat.ValidEmailCheck = IsDlgButtonChecked(hwnd, IDC_CHECKVALIDEMAIL);
                        user_dat.USBRefresh = IsDlgButtonChecked(hwnd, IDC_CHECKUSBREFRESH);
                        user_dat.TrayIcon = IsDlgButtonChecked(hwnd, IDC_CHECKMINTOTRAY);
                        user_dat.MAX_FAILED_EMAILS = (UINT)SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, 
                                                  TBM_GETPOS, (WPARAM)0, (LPARAM)0);
                        if (user_dat.TrayIcon) {
                            InitU2MTray(GetParent(hwnd));
                        } else {
                            DeleteU2MTrayIcon();
                            TrayIsInitialized = FALSE;
                        }
                        if (LOWORD(wParam) == IDOK) {
                            EndDialog(hwnd, (INT_PTR)TRUE);
                        }
                    } else {
                        ClearPrefs();
                    }
                    return (INT_PTR)TRUE;
                case IDCANCEL:
                    EndDialog(hwnd, (INT_PTR)TRUE);
                    return (INT_PTR)TRUE;
            }
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK USBDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LVCOLUMN vendCol, devcCol;
    static HANDLE refresh_usb_hnd;
    static HICON refreshDlgIco = NULL, usbDlgIco = NULL;
    static INT temp_idx = -1;

    switch (msg) {
        case WM_INITDIALOG:
            refreshDlgIco = (HICON)LoadImage(GetModuleHandle(NULL),
                                             MAKEINTRESOURCE(IDI_REFRESHICON),
                                             IMAGE_ICON, 18, 16, LR_LOADTRANSPARENT);
            if (refreshDlgIco) {
                SendDlgItemMessage(hwnd, IDUSBREFRESH, BM_SETIMAGE,
                                  (WPARAM)IMAGE_ICON, (LPARAM)refreshDlgIco);
            }

            usbDlgIco = (HICON)LoadImage(GetModuleHandle(NULL),
                                         MAKEINTRESOURCE(IDI_USB2MAILICONMEDIUM),
                                         IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                         GetSystemMetrics(SM_CYSMICON), 0);
            if (usbDlgIco) {
                SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)usbDlgIco);
            }

            SecureZeroMemory(&vendCol, sizeof(vendCol));
            SecureZeroMemory(&devcCol, sizeof(devcCol));
            temp_idx = -1;
            vendCol.mask = devcCol.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            vendCol.iSubItem = 0;
            devcCol.iSubItem = 1;
            vendCol.cx = devcCol.cx = 150;
            vendCol.fmt = LVCFMT_LEFT;
            devcCol.fmt = LVCFMT_RIGHT;
            {
                TCHAR tmpmsg1[255], tmpmsg2[255];
                LoadLocaleErrMsg(tmpmsg1, 29);
                LoadLocaleErrMsg(tmpmsg2, 28);
                devcCol.pszText = tmpmsg1;  //vendor
                vendCol.pszText = tmpmsg2;  //device
                ListView_InsertColumn(GetDlgItem(hwnd, IDC_USBDEVLIST), 0, &vendCol);
                ListView_InsertColumn(GetDlgItem(hwnd, IDC_USBDEVLIST), 1, &devcCol);
            }
            ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_USBDEVLIST),
                LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
            ListView_SetColumnWidth(GetDlgItem(hwnd, IDC_USBDEVLIST), 0, 210);
            ListView_SetColumnWidth(GetDlgItem(hwnd, IDC_USBDEVLIST), 1, 150);
            ListView_SetHoverTime(GetDlgItem(hwnd, IDC_USBDEVLIST), 1);
            CenterChild(hwnd);
            GetConnectedUSBDevs(hwnd, 0L, 0L, FILL_USB_LISTVIEW);
            if (user_dat.USBRefresh == TRUE) {
                ResetEvent(u2m_StartStop_event);
                refresh_usb_hnd = (HANDLE)_beginthreadex(NULL, 0, RefreshUSBThread, (LPVOID)hwnd, 0, NULL);
            }
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            usb_idx = 0;
            SecureZeroMemory(user_dat.usb_id_selection, sizeof(UINT)*2);
            switch (LOWORD(wParam)) {
                case IDUSBREFRESH:
                    ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_USBDEVLIST));
                    DeleteScannedUSBIDs();
                    GetConnectedUSBDevs(hwnd, 0L, 0L, FILL_USB_LISTVIEW);
                    if (temp_idx != -1)
                        ListView_SetItemState(GetDlgItem(hwnd, IDC_USBDEVLIST), (UINT)temp_idx,
                                              LVIS_FOCUSED | LVIS_SELECTED, 0x000f);
                    return (INT_PTR)TRUE;
                case IDOK:
                    if (!GetUSBListViewSelection(hwnd)) {
                        //if the message came from clicking the button then show the error messagebox
                        //if we double clicked and no item was under the mouse pointer (double clicked
                        //in white space) then there's no need to show the error message box
                        if (HIWORD(wParam) != 133) {
                            MessageBoxLocalized(hwnd, ID_ERR_MSG_27, ID_ERR_MSG_0, MB_OK | MB_ICONERROR);
                        }
                        return (INT_PTR)TRUE;
                    }
                case IDCANCEL:
                    if ((user_dat.USBRefresh == TRUE) && (refresh_usb_hnd != (HANDLE)NULL)) {
                        SetEvent(u2m_StartStop_event);
                        WaitForSingleObject(refresh_usb_hnd, INFINITE);
                        CloseHandle(refresh_usb_hnd);
                        refresh_usb_hnd = NULL;
                    }
                    DeleteScannedUSBIDs();
                    DestroyIcon(refreshDlgIco);
                    DestroyIcon(usbDlgIco);
                    EndDialog(hwnd, (INT_PTR)TRUE);
                    return (INT_PTR)TRUE;
            }
            return (INT_PTR)TRUE;
        case WM_NOTIFY:
            switch (LOWORD(wParam)) {
                case IDC_USBDEVLIST:
                    switch (((LPNMHDR)lParam)->code) {
                        case NM_DBLCLK:
                            //i'm using the high-order word value to pass a meaningless/random value
                            //to determine whether the message came from the click of the button
                            //or from double clicking a list item
                            SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDOK, 133), (LPARAM)0);
                            return (INT_PTR)TRUE;
                        case NM_CLICK:
                        case NM_RELEASEDCAPTURE:

                            temp_idx = (INT)SendMessage(GetDlgItem(hwnd, IDC_USBDEVLIST),
                                                        LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED);
                            if (temp_idx != -1)
                                ListView_SetItemState(GetDlgItem(hwnd, IDC_USBDEVLIST), (UINT)temp_idx,
                                                      LVIS_FOCUSED | LVIS_SELECTED, 0x000f);
                            return (INT_PTR)TRUE;
                    }
            }
            break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK EmailDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND FROM_ttip ATTRIB_UNUSED,
         TO_ttip ATTRIB_UNUSED,
         CC_ttip ATTRIB_UNUSED,
         SUBJECT_ttip ATTRIB_UNUSED,
         BODY_ttip ATTRIB_UNUSED;

    switch (msg) {
        case WM_INITDIALOG:
            //FROM_ttip = TO_ttip = CC_ttip = SUBJECT_ttip = BODY_ttip = NULL;
            {
                HICON emailDlgIco = (HICON)LoadImage(GetModuleHandle(NULL),
                                                 MAKEINTRESOURCE(IDI_EMAILICON),
                                                 IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                                 GetSystemMetrics(SM_CYSMICON), 0);
                if (emailDlgIco) {
                    SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)emailDlgIco);
                }

                TCHAR tmpmsg1[255], tmpmsg2[255], tmpmsg3[255], tmpmsg4[255], tmpmsg5[255];
                LoadLocaleErrMsg(tmpmsg1, 26);
                LoadLocaleErrMsg(tmpmsg2, 25);
                LoadLocaleErrMsg(tmpmsg3, 24);
                LoadLocaleErrMsg(tmpmsg4, 23);
                LoadLocaleErrMsg(tmpmsg5, 22);

                FROM_ttip = CreateBaloonToolTip(IDC_FROMFIELD, hwnd, tmpmsg1);
                TO_ttip = CreateBaloonToolTip(IDC_TOFIELD, hwnd, tmpmsg2);
                CC_ttip = CreateBaloonToolTip(IDC_CCFIELD, hwnd, tmpmsg3);
                SUBJECT_ttip = CreateBaloonToolTip(IDC_SUBJECTFIELD, hwnd, tmpmsg4);
                BODY_ttip = CreateBaloonToolTip(IDC_MESSAGEFIELD, hwnd, tmpmsg5);
            }
            if (user_dat.FROM)
                SetDlgItemTextA(hwnd, IDC_FROMFIELD, user_dat.FROM);
            if (user_dat.TO)
                SetDlgItemTextA(hwnd, IDC_TOFIELD, user_dat.TO);
            if (user_dat.CC)
                SetDlgItemTextA(hwnd, IDC_CCFIELD, user_dat.CC);
            if (user_dat.SUBJECT)
                SetDlgItemTextA(hwnd, IDC_SUBJECTFIELD, user_dat.SUBJECT);
            if (user_dat.BODY)
                SetDlgItemTextA(hwnd, IDC_MESSAGEFIELD, user_dat.BODY);
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    ClearEmailData();
                    if (!parseEmailDialogFields(hwnd)) {
                        ClearEmailData();
                        break;
                    }
                case IDCANCEL:
                    /*DestroyWindow(FROM_ttip);
                    DestroyWindow(TO_ttip);
                    DestroyWindow(CC_ttip);
                    DestroyWindow(SUBJECT_ttip);
                    DestroyWindow(BODY_ttip);*/
                    EndDialog(hwnd, (INT_PTR)TRUE);
                    return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK HelpDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
            HlpDlg_open = TRUE;
            {
                HICON helpDlgIco = LoadIcon(NULL, IDI_QUESTION);
                if (helpDlgIco) {
                    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)helpDlgIco);
                }

                TCHAR tmp1[7500], tmp2[2500];
                LoadString(*g_hInst, ID_HELP_MSG1, tmp1, sizeof(tmp1) / sizeof(tmp1[0]));
                LoadString(*g_hInst, ID_HELP_MSG2, tmp2, sizeof(tmp2) / sizeof(tmp2[0]));
                StringCchCat(tmp1, 7500, tmp2);
                SetDlgItemText(hwnd, IDC_HELP_TEXT, tmp1);
            }
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    HlpDlg_open = FALSE;
                    DestroyWindow(hwnd);
                    return (INT_PTR)TRUE;
            }
            break;
        case WM_CLOSE:
            HlpDlg_open = FALSE;
            DestroyWindow(hwnd);
            return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT mainwindowcontrol_font_big, mainwindowcontrol_font;
    HDC hdc;
    PAINTSTRUCT ps;

    switch (msg) {
        case WM_CREATE:
            if (user_dat.TrayIcon) InitU2MTray(hwnd);
            {
                TCHAR tmp1[255], tmp2[255], tmp3[255], tmp4[255];
                LoadLocaleErrMsg(tmp1, 21);
                LoadLocaleErrMsg(tmp2, 19);
                LoadLocaleErrMsg(tmp3, 9);
                LoadLocaleErrMsg(tmp4, 15);

                mainwindowcontrol_font_big = CreateFont(
                                24, 0, //height, width
                                0, 0, //escapement, orientation angles
                                FW_DONTCARE, //boldness 0-1000, 400 = normal, 700 = bold
                                FALSE, FALSE, FALSE, //italics, underline, strikeout
                                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, //charset, output precision
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //clipping, output quality
                                VARIABLE_PITCH, TEXT("Trebuchet MS"));

                mainwindowcontrol_font = CreateFont(18, 0, 0, 0, 550,
                                FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                VARIABLE_PITCH, TEXT("Trebuchet MS"));

                USBListButton = CreateWindowEx(0, TEXT("BUTTON"), tmp1,
                                 WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                 30, 30, 200, 30, hwnd, (HMENU)IDC_CHOOSEUSBBUTTON,
                                 (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
                if (!USBListButton) {
                    MessageBoxLocalized(hwnd, ID_ERR_MSG_20, ID_ERR_MSG_31, MB_OK | MB_ICONERROR);
                    return -2;
                }
                SendMessage(USBListButton, WM_SETFONT, (WPARAM)mainwindowcontrol_font, (LPARAM)TRUE);

                EMAILButton = CreateWindowEx(0, TEXT("BUTTON"), tmp2,
                                 WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                 300, 30, 200, 30, hwnd, (HMENU)IDC_EMAILBUTTON,
                                 (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
                if (!EMAILButton) {
                    MessageBoxLocalized(hwnd, ID_ERR_MSG_18, ID_ERR_MSG_31, MB_OK | MB_ICONERROR);
                    return -2;
                }
                SendMessage(EMAILButton, WM_SETFONT, (WPARAM)mainwindowcontrol_font, (LPARAM)TRUE);

                STARTSTOP = CreateWindowEx(0, TEXT("BUTTON"), tmp3,
                                 WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                 300, 200, 200, 50, hwnd, (HMENU)IDC_STARTSTOP,
                                 *g_hInst, NULL);
                if (!STARTSTOP) {
                    MessageBoxLocalized(hwnd, ID_ERR_MSG_17, ID_ERR_MSG_31, MB_OK | MB_ICONERROR);
                    return -2;
                }
                SendMessage(STARTSTOP, WM_SETFONT, (WPARAM)mainwindowcontrol_font_big, (LPARAM)TRUE);

                time_track = CreateWindowEx(0, TRACKBAR_CLASS, TEXT(""),
                                 WS_CHILD | WS_VISIBLE | TBS_TOOLTIPS | TBS_NOTICKS | TBS_HORZ,
                                 30, 200, 200, 30, hwnd, (HMENU)IDC_TIMETRACK,
                                 *g_hInst, NULL);
                if (!time_track) {
                    MessageBoxLocalized(hwnd, ID_ERR_MSG_16, ID_ERR_MSG_31, MB_OK | MB_ICONERROR);
                    return -2;
                }

                ttrack_label = CreateWindow(TEXT("STATIC"), tmp4,
                                 WS_VISIBLE | WS_CHILD | SS_CENTER,
                                 30, 160, 200, 40, hwnd, NULL, *g_hInst, NULL);
                if (!ttrack_label) {
                    MessageBoxLocalized(hwnd, ID_ERR_MSG_14, ID_ERR_MSG_31, MB_OK | MB_ICONERROR);
                    return -2;
                }
                SendMessage(ttrack_label, WM_SETFONT, (WPARAM)mainwindowcontrol_font, (LPARAM)TRUE);

                SendMessage(time_track, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ttrack_struct);
                SendMessage(time_track, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(T_MIN, T_MAX));
                SendMessage(time_track, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)4);
                SendMessage(time_track, TBM_SETSEL, (WPARAM)FALSE, (LPARAM)MAKELONG(T_MIN, T_MAX));
                SendMessage(time_track, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)user_dat.TIMEOUT);

                if (!user_dat.Autostart) {
                    UpdateWindow(hwnd);
                } else {
                    SendMessage(hwnd, WM_COMMAND, MAKEWPARAM((WORD)IDC_STARTSTOP, 0), (LPARAM)0);
                }
                CheckMenuItem(GetMenu(hwnd), IDM_AUTOSTART,
                              (user_dat.Autostart)?(MF_BYCOMMAND | MF_CHECKED):(MF_BYCOMMAND | MF_UNCHECKED));
                DrawMenuBar(hwnd);
            }
        case WM_PAINT:
            if ((hdc = BeginPaint(hwnd, &ps))) {
                EndPaint(hwnd, &ps);
            }
            break;
        /* custom message to use from the U2MThread */
        case WM_ENABLE_STARTSTOP:
            EnableWindow(STARTSTOP, !IsWindowEnabled(STARTSTOP));
            break;
        case WM_STARTSTOP_CONTROL:
            PostMessage(hwnd, WM_COMMAND, MAKEWPARAM((WORD)IDC_STARTSTOP, 0), (LPARAM)0);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_EN_LANG:
                case IDM_GR_LANG:
                    currentLangID = LOWORD(wParam);
                    ResetLocale(hwnd);
                    break;
                case IDM_ABOUT:
                    InitAboutDialog(hwnd);
                    break;
                case IDM_PREF:
                    if (!onoff)
                        InitPreferencesDialog(hwnd);
                    else
                        MessageBoxLocalized(hwnd, ID_ERR_MSG_13, ID_ERR_MSG_1, MB_OK | MB_ICONEXCLAMATION);
                    break;
                case IDM_AUTOSTART:
                    if (AutostartWarning) {
                        AutostartWarning = FALSE;
                        MessageBoxLocalized(hwnd, ID_ERR_MSG_62, ID_ERR_MSG_63, MB_ICONASTERISK | MB_OK);
                    }
                    user_dat.Autostart = !user_dat.Autostart;
                    CheckMenuItem(GetMenu(hwnd), IDM_AUTOSTART,
                                  (user_dat.Autostart)?(MF_BYCOMMAND | MF_CHECKED):(MF_BYCOMMAND | MF_UNCHECKED));
                    DrawMenuBar(hwnd);
                    break;
                case IDM_PASSWORD:
                    if (!onoff)
                        InitPasswordDialog(hwnd);
                    else
                        MessageBoxLocalized(hwnd, ID_ERR_MSG_12, ID_ERR_MSG_1, MB_OK | MB_ICONEXCLAMATION);
                    break;
                case IDC_CHOOSEUSBBUTTON:
                    if (!onoff)
                        InitUSBDialog(hwnd);
                    else
                        MessageBoxLocalized(hwnd, ID_ERR_MSG_11, ID_ERR_MSG_1, MB_OK | MB_ICONEXCLAMATION);
                    break;
                case IDC_STARTSTOP:
                    if (onoff) {
                        SetEvent(u2m_StartStop_event);
                        if (u2mMainThread) {
                            WaitForSingleObject(u2mMainThread, INFINITE);
                            CloseHandle(u2mMainThread);
                            u2mMainThread = NULL;
                            FreeModuleHeap();
                        }
                    } else {
                        ResetEvent(u2m_StartStop_event);
                    }
                    onoff = InitU2MThread(&user_dat, hwnd);
                    EnableWindow(USBListButton, !onoff);
                    EnableWindow(EMAILButton, !onoff);
                    EnableWindow(time_track, !onoff);
                    EnableWindow(ttrack_label, !onoff);
                    {
                        TCHAR tmp[255];
                        UINT uID = (!onoff)?ID_ERR_MSG_9:ID_ERR_MSG_10;
                        LoadString(*g_hInst, uID, tmp, sizeof(tmp)/sizeof(tmp[0]));
                        SetWindowText(STARTSTOP, tmp);
                    }
                    SetU2MNotifyTip();
                    break;
                case IDC_EMAILBUTTON:
                    if (!onoff)
                        InitEmailDialog(hwnd);
                    else
                        MessageBoxLocalized(hwnd, ID_ERR_MSG_8, ID_ERR_MSG_1, MB_OK | MB_ICONEXCLAMATION);
                    break;
                case IDM_H_ELP1:
                    InitHelpDialog(hwnd);
                    break;
                case IDM_TRAY_OPENWINDOW:
                    if (!IsWindowVisible(hwnd)) {
                        ShowWindow(hwnd, SW_SHOW);
                    }
                    SetFocus(hwnd);
                    break;
                case IDM_TRAY_QUIT:
                case IDM_EXIT:
                    PostMessage(hwnd, WM_CLOSE, wParam, lParam);
                    break;
            }
            break;
        case WM_SYSCOMMAND:
            switch (wParam) {
                case SC_MINIMIZE:
                    if (user_dat.TrayIcon) {
                        ShowWindow(hwnd, SW_HIDE);
                    } else {
                        ShowWindow(hwnd, SW_MINIMIZE);
                    }
                    break;
                default:
                    return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            break;
        /* custom message for handling the messages for the USB2Email Tray icon */
        case WM_U2M_NOTIF_ICON:
            switch (lParam) {
                case WM_RBUTTONUP:
                    {
                        POINT cursor_pos;
                        TCHAR tmpmsg1[255], tmpmsg2[255], tmpmsg3[255] ATTRIB_UNUSED;

                        (VOID)tmpmsg3;
                        LoadLocaleErrMsg(tmpmsg1, 60);
                        LoadLocaleErrMsg(tmpmsg2, 61);
                        //LoadLocaleErrMsg(tmpmsg3, 10);
                        TrayIconMenu = CreatePopupMenu();
                        AppendMenu(TrayIconMenu, MF_STRING, IDM_TRAY_OPENWINDOW, tmpmsg2);
                        //AppendMenu(TrayIconMenu, MF_STRING | ((onoff) ? MF_ENABLED : MF_DISABLED), IDC_STARTSTOP, tmpmsg3);
                        AppendMenu(TrayIconMenu, MF_SEPARATOR, 0, NULL);
                        AppendMenu(TrayIconMenu, MF_STRING, IDM_TRAY_QUIT, tmpmsg1);

                        GetCursorPos(&cursor_pos);
                        SetForegroundWindow(hwnd);
                        TrackPopupMenuEx(TrayIconMenu,
                                 TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION,
                                 cursor_pos.x, cursor_pos.y, hwnd, NULL);
                    }
                    break;
                case WM_LBUTTONUP:
                    if (!IsWindowVisible(hwnd)) {
                        ShowWindow(hwnd, SW_SHOW);
                        SetForegroundWindow(hwnd);
                    } else ShowWindow(hwnd, SW_HIDE);
                    break;
            }
            break;
        case WM_CLOSE:
            if (onoff) {
                MessageBoxLocalized(hwnd, ID_ERR_MSG_7, ID_ERR_MSG_1, MB_OK | MB_ICONEXCLAMATION);
                break;
            }
#ifndef DEBUG
            if (MessageBoxLocalized(hwnd, ID_ERR_MSG_6, ID_ERR_MSG_5, MB_ICONASTERISK | MB_YESNO) == IDNO)
                break;
#endif
            if (!WriteDataToU2MReg(&user_dat)) {
                MessageBoxLocalized(hwnd, ID_ERR_MSG_66, ID_ERR_MSG_31, MB_ICONERROR);
            }
        case WM_QUIT:
            DeleteAll();
            DeleteObject(mainwindowcontrol_font_big);
            DeleteObject(mainwindowcontrol_font);
            DestroyWindow(hwnd);
            break;
        case WM_HSCROLL:
            user_dat.TIMEOUT = (UINT)SendMessage(time_track, TBM_GETPOS, (WPARAM)0, (LPARAM)0);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;
    BOOL bRet = TRUE;
    RECT wrkspace_px;
    INT center_x, center_y;
    INT err;
    HANDLE u2m_sinstance_mtx = NULL;
    INITCOMMONCONTROLSEX columnControlClass = {sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS};
#if !_MSC_VER
    HMODULE libcurl_dll;
    int (*curl_global_init)(long);
    void (*curl_global_cleanup)(void);
#endif

    // check if there's already an instance of the application running
    u2m_sinstance_mtx = InitSingleInstanceMutex(); // possible DOS?
    err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS || err == ERROR_ACCESS_DENIED || u2m_sinstance_mtx == NULL) {
        MessageBox(NULL, TEXT("There's already a running instance of USB2Email!"),
                         TEXT("USB2Email is open!"), MB_ICONERROR);
        return 1;
    }

    u2m_StartStop_event = InitStartStopEvent();
    if (!u2m_StartStop_event) {
        __MsgBoxGetLastError(NULL, TEXT("InitStartStopEvent()"), __LINE__);
        CloseHandle(u2m_sinstance_mtx);
        return 1;
    }

#if !_MSC_VER //this is really just if you compile dynamically with the libcurl DLL libquickmail depends on
    libcurl_dll = LoadLibrary(TEXT("libcurl-4.dll"));
    if (!libcurl_dll) {
        __MsgBoxGetLastError(NULL, TEXT("LoadLibrary(TEXT(\"libcurl-4.dll\"))"), __LINE__);
        CloseHandle(u2m_StartStop_event);
        CloseHandle(u2m_sinstance_mtx);
        return 1;
    }
    curl_global_init = (int(*)(long))GetProcAddress(libcurl_dll, "curl_global_init");
    curl_global_cleanup = (void(*)(void))GetProcAddress(libcurl_dll, "curl_global_cleanup");
#endif
    curl_global_init(CURL_GLOBAL_SSL | CURL_GLOBAL_WIN32);

    g_hInst = &hInstance; //possibly a bad decision
    err = LoadLocaleDLLs();
    if (!err) {
        CloseHandle(u2m_StartStop_event);
        CloseHandle(u2m_sinstance_mtx);
        return 1;
    }

    InitCommonControlsEx(&columnControlClass);

    InitU2MLogging();
    /*** Global initializations ***/
    onoff = FALSE;
    TrayIconMenu = NULL;
    SecureZeroMemory(user_dat.usb_id_selection, sizeof(UINT)*2);

    usb_idx = 0;

    u2mMainThread = NULL;

    parseConfFile(&user_dat);
    err = GetU2MRegData(&user_dat);
    if (!err) MessageBoxLocalized(NULL, ID_ERR_MSG_65, ID_ERR_MSG_31, MB_ICONERROR);

    if (user_dat.Autostart) user_dat.TrayIcon = TRUE;


    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = wc.cbClsExtra = wc.cbWndExtra = 0;
    wc.lpfnWndProc = MainWindowProcedure;
    wc.hInstance = *g_hInst;
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USB2MAILICONMEDIUM));
    wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_USB2MAILICONSMALL));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wc.lpszClassName = szClassName;
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MAINMENU);

    /* Get the coords of the screen without the taskbar */
    SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID)&wrkspace_px, SPIF_SENDCHANGE);

    /* Coords to center the window in the center of the working area */
    center_x = (wrkspace_px.right - 550)/2;
    center_y = (wrkspace_px.bottom - 350)/2;

    if (!RegisterClassEx(&wc)) {
        MessageBoxLocalized(NULL, ID_ERR_MSG_4, ID_ERR_MSG_31, MB_ICONERROR);
        DestroyLanguageLibraries(ENGLISH_DLL);
        CloseHandle(u2m_StartStop_event);
        CloseHandle(u2m_sinstance_mtx);
        curl_global_cleanup();
        return -1;
    }

    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, szClassName, TEXT("USB2Email"),
                          WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_EX_LAYERED,
                          center_x, center_y, 550, 350, NULL, NULL, *g_hInst, NULL);

    if (!hwnd) {
        MessageBoxLocalized(NULL, ID_ERR_MSG_3, ID_ERR_MSG_31, MB_ICONERROR);
        DestroyLanguageLibraries(ENGLISH_DLL);
        CloseHandle(u2m_StartStop_event);
        CloseHandle(u2m_sinstance_mtx);
        curl_global_cleanup();
        return -2;
    }

    if (!user_dat.Autostart) {
#if 1
        ShowWindow(hwnd, SW_SHOW);
        SetActiveWindow(hwnd);
#else //GDI usage errors with AnimateWindow!?
        AnimateWindow(hwnd, 200, AW_CENTER | AW_ACTIVATE);
        //PrintWindow(hwnd, GetWindowDC(hwnd), 0);
#endif
    }

    UpdateWindow(hwnd);

    while (bRet) {
        bRet = GetMessage(&Msg, NULL, 0, 0);

        if (bRet == -1) {
            MessageBoxLocalized(hwnd, ID_ERR_MSG_2, ID_ERR_MSG_0, MB_ICONERROR);
            break;
        }

        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    curl_global_cleanup();

#if !_MSC_VER
    FreeLibrary(libcurl_dll);
#endif
    CloseHandle(u2m_StartStop_event);
    CloseHandle(u2m_sinstance_mtx);
    return Msg.wParam;
}
