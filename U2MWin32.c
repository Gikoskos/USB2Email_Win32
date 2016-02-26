/******************************************
*                U2MWin32.c                *
*        George Koskeridis (C)2015         *
 ******************************************/

#include "U2MWin32.h"

const _TCHAR szClassName[] = _T("USB2Email");

HINSTANCE *g_hInst;

ULONG usb_id_selection[2];

/********************
*Input field raw data*
 ********************/
char *pass, *FROM, *TO, *CC, *SUBJECT, *BODY, *SMTP_SERVER;
UINT PORT;

/*******************************
*Bools to check enabled controls*
 *******************************/
BOOL ValidEmailCheck = FALSE; //for the valid e-mail check control
BOOL USBRefresh = FALSE; //for the USB refresh check control
BOOL TrayIcon = FALSE; //for the Tray Icon check control
BOOL USBdev_scan = FALSE; //helper global for the USB reload thread
BOOL HlpDlg_open = FALSE; //bool to check whether Help dialog has already been opened
BOOL TrayIsInitialized = FALSE; //saves memory by initializing the tray icon once at any point

HDC hdc;
PAINTSTRUCT ps;

/*Is TRUE when the service is running and FALSE when it's not*/
UINT onoff = FALSE;

/*Main Window controls*/
WNDCLASSEX wc; //window class
HWND USBListButton, EMAILButton, STARTSTOP, time_track, ttrack_tooltip, ttrack_label;
TOOLINFO ttrack_struct;
HMENU MainMenu, TrayIconMenu; //to menu sto kyrio parathyro kai to menu tou tray icon
NOTIFYICONDATA U2MTrayData = {0}; //tray area icon

UINT MAX_FAILED_EMAILS = 0;
UINT TIMEOUT;
UINT usb_idx;


struct dll_data {
    TCHAR *filename;
    HMODULE module;
    HMENU locale_menu;
};

struct {
    struct dll_data U2MLocale_en;
    struct dll_data U2MLocale_gr;
} U2M_dlls = {{_T("U2MLocale_en.dll"), NULL, NULL},
              {_T("U2MLocale_gr.dll"), NULL, NULL}};

WORD currentLangID;


/*Trackbar limits*/
#define T_MIN                     200
#define T_MAX                    2000

/*Flag for ValidEmailCheck*/
#define NO_SEPARATOR              '\0'


/*******************************************
* Prototypes for functions with local scope *
 *******************************************/
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
VOID ResetMainWindowLanguage(HWND hwnd);

VOID InitEmailDialog(HWND hwnd);
VOID InitAboutDialog(HWND hwnd);
VOID InitPasswordDialog(HWND hwnd);
VOID InitPreferencesDialog(HWND hwnd);
VOID InitHelpWindow(HWND hwnd);
VOID CenterChild(HWND hwnd);
VOID InitU2MTray(HWND hwnd);
BOOL InitU2MTrayMenu(VOID);
VOID SetU2MNotifyTip(VOID);

UINT CALLBACK RefreshUSBThread(LPVOID dat);
VOID AddDeviceToUSBListView(HWND hDlg, char *dev_str, char *ven_str);
VOID DeleteAll(VOID);
VOID GetFieldTextA(HWND hwnd, int nIDDlgItem, char **str);
VOID GetFieldText(HWND hwnd, int nIDDlgItem, TCHAR **str);

//@TODO: Implement ErrorCodes function

VOID DeleteAll(VOID)
{
    ClearEmailData();
    ClearPwd();
    ClearPrefs();
    DeleteU2MTrayIcon();
}

VOID InitPreferencesDialog(HWND hwnd)
{
    if (!DialogBoxParam(*g_hInst, MAKEINTRESOURCE(IDD_PREFDIALOG), hwnd, PrefDialogProcedure, (LPARAM)0)) {
        TCHAR tmp1[255], tmp2[255];
        LoadString(*g_hInst, ID_ERR_MSG_52, tmp1, sizeof(tmp1)/sizeof(tmp1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmp2, sizeof(tmp2)/sizeof(tmp2[0]));
        MessageBoxEx(hwnd, tmp1, tmp2, MB_OK | MB_ICONERROR, currentLangID);
    }
}

VOID InitPasswordDialog(HWND hwnd)
{
    if (!DialogBoxParam(*g_hInst, MAKEINTRESOURCE(IDD_PWDDIALOG), hwnd, PwdDialogProcedure, (LPARAM)0)) {
        TCHAR tmp1[255], tmp2[255];
        LoadString(*g_hInst, ID_ERR_MSG_51, tmp1, sizeof(tmp1)/sizeof(tmp1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmp2, sizeof(tmp2)/sizeof(tmp2[0]));
        MessageBoxEx(hwnd, tmp1, tmp2, MB_OK | MB_ICONERROR, currentLangID);
    }
}

VOID InitUSBDialog(HWND hwnd)
{
    if (!DialogBoxParam(*g_hInst, MAKEINTRESOURCE(IDD_USBDIALOG), hwnd, USBDialogProcedure, (LPARAM)0)) {
        TCHAR tmp1[255], tmp2[255];
        LoadString(*g_hInst, ID_ERR_MSG_50, tmp1, sizeof(tmp1)/sizeof(tmp1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmp2, sizeof(tmp2)/sizeof(tmp2[0]));
        MessageBoxEx(hwnd, tmp1, tmp2, MB_OK | MB_ICONERROR, currentLangID);
    }
}

VOID InitAboutDialog(HWND hwnd)
{
    if (!DialogBoxParam(*g_hInst, MAKEINTRESOURCE(IDD_ABOUTDIALOG), hwnd, AboutDialogProcedure, (LPARAM)0)) {
        TCHAR tmp1[255], tmp2[255];
        LoadString(*g_hInst, ID_ERR_MSG_49, tmp1, sizeof(tmp1)/sizeof(tmp1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmp2, sizeof(tmp2)/sizeof(tmp2[0]));
        MessageBoxEx(hwnd, tmp1, tmp2, MB_OK | MB_ICONERROR, currentLangID);
    }
}

VOID InitEmailDialog(HWND hwnd)
{
    if (!DialogBoxParam(*g_hInst, MAKEINTRESOURCE(IDD_EMAILDIALOG), hwnd, EmailDialogProcedure, (LPARAM)0)) {
        TCHAR tmp1[255], tmp2[255];
        LoadString(*g_hInst, ID_ERR_MSG_48, tmp1, sizeof(tmp1)/sizeof(tmp1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmp2, sizeof(tmp2)/sizeof(tmp2[0]));
        MessageBoxEx(hwnd, tmp1, tmp2, MB_OK | MB_ICONERROR, currentLangID);
    }
}

VOID InitHelpDialog(HWND hwnd)
{
    if (!HlpDlg_open) {
        HRSRC HelpDlgHrsrc = FindResource(*g_hInst, MAKEINTRESOURCE(IDD_HELPDIALOG), 
                                          MAKEINTRESOURCE(RT_DIALOG));
        HGLOBAL HelpDlgHandle = LoadResource(*g_hInst, HelpDlgHrsrc);
        LPCDLGTEMPLATE HelpDlgPtr = (LPCDLGTEMPLATE)LockResource(HelpDlgHandle);

        if (!CreateDialogIndirectParam(*g_hInst, HelpDlgPtr, hwnd, HelpDialogProcedure, (LPARAM)0)) {
            TCHAR tmp1[255], tmp2[255];
            LoadString(*g_hInst, ID_ERR_MSG_47, tmp1, sizeof(tmp1)/sizeof(tmp1[0]));
            LoadString(*g_hInst, ID_ERR_MSG_0, tmp2, sizeof(tmp2)/sizeof(tmp2[0]));
            MessageBoxEx(hwnd, tmp1, tmp2, MB_OK | MB_ICONERROR, currentLangID);
        }
    } else {
        SetActiveWindow(GetDlgItem(hwnd, IDD_HELPDIALOG));
    }
}

VOID ResetMainWindowLanguage(HWND hwnd)
{
    TCHAR tmp1[255], tmp2[255], tmp3[255], tmp4[255];

    LoadString(*g_hInst, ID_ERR_MSG_21, tmp1, sizeof(tmp1)/sizeof(tmp1[0]));
    LoadString(*g_hInst, ID_ERR_MSG_19, tmp2, sizeof(tmp2)/sizeof(tmp2[0]));
    LoadString(*g_hInst, ID_ERR_MSG_9, tmp3, sizeof(tmp3)/sizeof(tmp3[0]));
    LoadString(*g_hInst, ID_ERR_MSG_15, tmp4, sizeof(tmp4)/sizeof(tmp4[0]));

    SetWindowText(USBListButton, tmp1);
    SetWindowText(EMAILButton, tmp2);
    SetWindowText(STARTSTOP, tmp3);
    SetWindowText(ttrack_label, tmp4);
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
        (*str) = calloc(bufsiz, 1);
        GetDlgItemTextA(hwnd, nIDDlgItem, (*str), bufsiz);
    } else {
        if (*str)
            free(*str);
        (*str) = NULL;
    }
}

VOID GetFieldText(HWND hwnd, int nIDDlgItem, TCHAR **str)
{
    int bufsiz = GetWindowTextLengthA(GetDlgItem(hwnd, nIDDlgItem)) + 1;

    if (bufsiz > 1) {
        (*str) = calloc(bufsiz, sizeof(TCHAR));
        GetDlgItemText(hwnd, nIDDlgItem, (*str), bufsiz);
    } else {
        if (*str)
            free(*str);
        (*str) = NULL;
    }
}

HWND WINAPI CreateBaloonToolTip(int toolID, HWND hDlg, TCHAR *pszText)
{
    if (!toolID || !hDlg || !pszText)
        return NULL;
    HWND hwndTool = GetDlgItem(hDlg, toolID);

    HWND hwndTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
                                  WS_POPUP |TTS_ALWAYSTIP | TTS_BALLOON,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  CW_USEDEFAULT, CW_USEDEFAULT,
                                  hDlg, NULL, 
                                  *g_hInst, NULL);

    if (!hwndTool || !hwndTip)
        return NULL;             

    TOOLINFO toolInfo = { 0 };
    toolInfo.cbSize = sizeof(toolInfo) - sizeof(void*);
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
                                 hDlg, NULL, *g_hInst,NULL);

    if (!hwndTT)
        return NULL;

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

    GetFieldTextA(hwnd, IDC_SERVERURLFIELD, &tmp1);
    if (!tmp1 && !onoff) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_46, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
        return FALSE;
    }

    GetFieldTextA(hwnd, IDC_PORTFIELD, &tmp2);

    if (tmp2) {
        if (strlen(tmp2) > 5) {
            free(tmp1);
            free(tmp2);
            TCHAR tmpmsg1[255], tmpmsg2[255];
            LoadString(*g_hInst, ID_ERR_MSG_44, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
            LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
            MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
            return FALSE;
        }

        PORT = 0;
        UINT c;
        DOUBLE temp_len = (DOUBLE)strlen(tmp2);
        for (size_t i = 0; i < strlen(tmp2) ; i++) {
            c = tmp2[i] - '0';
            PORT = c*((UINT)pow(10, (temp_len - (i + 1)))) + PORT;
        }
        free(tmp2);

        if (PORT > 65535) {
            free(tmp1);
            TCHAR tmpmsg1[255], tmpmsg2[255];
            LoadString(*g_hInst, ID_ERR_MSG_44, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
            LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
            MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
            return FALSE;
        }
    } else {
        PORT = 0;
        /*free(tmp1);
        free(tmp2);
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_45, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);*/
    }

    SMTP_SERVER = realloc(NULL, strlen(tmp1)+1);
    snprintf(SMTP_SERVER, strlen(tmp1)+1, "%s", tmp1);
    free(tmp1);
    return TRUE;
}

BOOL parseEmailDialogFields(HWND hwnd)
{
    char *tmp = NULL;


    GetFieldTextA(hwnd, IDC_FROMFIELD, &tmp);
    if (!tmp) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_43, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
        return FALSE;
    }

    if (ValidEmailCheck) { 
        if (!isValidDomain(tmp, NO_SEPARATOR)) {
            TCHAR tmpmsg1[255], tmpmsg2[255];
            LoadString(*g_hInst, ID_ERR_MSG_42, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
            LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
            MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
            free(tmp);
            return FALSE;
        }
    }
    FROM = realloc(NULL, strlen(tmp)+1);
    snprintf(FROM, strlen(tmp)+1, "%s", tmp);
    free(tmp);
    tmp = NULL;

    GetFieldTextA(hwnd, IDC_TOFIELD, &tmp);
    if (!tmp) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_41, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
        return FALSE;
    }

    if (ValidEmailCheck) { 
        if (!isValidDomain(tmp, NO_SEPARATOR)) {
            free(tmp);
            TCHAR tmpmsg1[255], tmpmsg2[255];
            LoadString(*g_hInst, ID_ERR_MSG_40, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
            LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
            MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
            return FALSE;
        }
    }
    TO = realloc(NULL, strlen(tmp)+1);
    snprintf(TO, strlen(tmp)+1, "%s", tmp);
    free(tmp);
    tmp = NULL;

    GetFieldTextA(hwnd, IDC_CCFIELD, &tmp);

    if (ValidEmailCheck && tmp) { 
        if (!isValidDomain(tmp, ';')) {
            free(tmp);
            TCHAR tmpmsg1[255], tmpmsg2[255];
            LoadString(*g_hInst, ID_ERR_MSG_39, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
            LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
            MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
            return FALSE;
        }
    } else if (!tmp) {
        CC = NULL;
    } else {
        CC = realloc(NULL, strlen(tmp)+1);
        strncpy(CC, tmp, strlen(tmp)+1);
        free(tmp);
        tmp = NULL;
    }

    GetFieldTextA(hwnd, IDC_SUBJECTFIELD, &tmp);
    if (!tmp) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_38, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_37, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        if (MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_YESNO | MB_ICONASTERISK, currentLangID) == IDYES) {
            SUBJECT = malloc(2);
            snprintf(SUBJECT, 2, "");
        } else {
            return FALSE;
        }
    } else {
        SUBJECT = realloc(NULL, strlen(tmp)+1);
        strncpy(SUBJECT, tmp, strlen(tmp)+1);
        free(tmp);
        tmp = NULL;
    }

    GetFieldTextA(hwnd, IDC_MESSAGEFIELD, &tmp);
    if (!tmp) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_36, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_35, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        if (MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_YESNO | MB_ICONASTERISK, currentLangID) == IDYES) {
            BODY = malloc(2);
            snprintf(BODY, 2, "");
        } else {
            return FALSE;
        }
    } else {
        BODY = realloc(NULL, strlen(tmp)+1);
        strncpy(BODY, tmp, strlen(tmp)+1);
        free(tmp);
    }

    return TRUE;
}

BOOL parsePwdField(HWND hwnd)
{
    char *tmp = NULL;

    GetFieldTextA(hwnd, IDC_PWDFIELD, &tmp);
    if (!tmp) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_34, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_OK | MB_ICONERROR, currentLangID);
        return FALSE;
    } else {
        pass = realloc(NULL, strlen(tmp)+1);
        strncpy(pass, tmp, strlen(tmp)+1);
        free(tmp);
    }
    return TRUE;
}

VOID AddDeviceToUSBListView(HWND hDlg, char *dev_str, char *ven_str)
{
    LVITEM dev;

    if (!dev_str) {
        TCHAR tmpmsg[255];
        LoadString(*g_hInst, ID_ERR_MSG_33, tmpmsg, sizeof(tmpmsg)/sizeof(tmpmsg[0]));
        dev.pszText = tmpmsg;
    } else {
#if defined(UNICODE) || defined(_UNICODE)
        wchar_t wc_temp[255];
        mbstowcs(wc_temp, dev_str, strlen(dev_str) + 1);
        dev.pszText = wc_temp;
#else
        dev.pszText = dev_str;
#endif
    }
    dev.mask = LVIF_TEXT | LVIF_STATE;
    dev.state = 0;
    dev.stateMask = 0;
    dev.iSubItem = 0;
    dev.iItem = usb_idx;
    ListView_InsertItem(GetDlgItem(hDlg, IDC_USBDEVLIST), &dev);
    if (!ven_str) {
        TCHAR tmpmsg[255];
        LoadString(*g_hInst, ID_ERR_MSG_32, tmpmsg, sizeof(tmpmsg)/sizeof(tmpmsg[0]));
        ListView_SetItemText(GetDlgItem(hDlg, IDC_USBDEVLIST), usb_idx, 1, tmpmsg);
    } else {
#if defined(UNICODE) || defined(_UNICODE)
        wchar_t wc_temp[255];
        mbstowcs(wc_temp, ven_str, strlen(ven_str) + 1);
        ListView_SetItemText(GetDlgItem(hDlg, IDC_USBDEVLIST), usb_idx, 1, wc_temp);
#else
        ListView_SetItemText(GetDlgItem(hDlg, IDC_USBDEVLIST), usb_idx, 1, ven_str);
#endif
    }
    usb_idx++;
}

BOOL GetUSBListViewSelection(HWND hwnd)
{
    INT usb_sel_idx = (INT)SendMessage(GetDlgItem(hwnd, IDC_USBDEVLIST), 
                                       LVM_GETNEXTITEM, -1, LVNI_SELECTED);

    if (usb_sel_idx == -1) return FALSE;

    usb_id_selection[0] = scanned_usb_ids[usb_sel_idx][0];
    usb_id_selection[1] = scanned_usb_ids[usb_sel_idx][1];
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
        LoadString(*g_hInst, ID_ERR_MSG_58, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        _tcscpy(U2MTrayData.szTip, tmpmsg1);
        Shell_NotifyIcon(NIM_ADD, &U2MTrayData);
        Shell_NotifyIcon(NIM_SETVERSION, &U2MTrayData);
    } else {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_30, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_31, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(NULL, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);            
    }
}

VOID SetU2MNotifyTip(VOID)
{
    if (TrayIsInitialized && TrayIcon) {
        if (onoff) {
            TCHAR tmpmsg1[50];
            LoadString(*g_hInst, ID_ERR_MSG_59, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
            _tcscpy(U2MTrayData.szTip, tmpmsg1);
            Shell_NotifyIcon(NIM_MODIFY, &U2MTrayData);
        } else {
            TCHAR tmpmsg1[50];
            LoadString(*g_hInst, ID_ERR_MSG_58, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
            _tcscpy(U2MTrayData.szTip, tmpmsg1);
            Shell_NotifyIcon(NIM_MODIFY, &U2MTrayData); 
        }
    }
}

UINT CALLBACK RefreshUSBThread(LPVOID dat)
{
    HWND hwnd = (HWND)dat;

    while (USBdev_scan) {
        Sleep(2000);
        SendMessageTimeout(hwnd, WM_COMMAND, 
                           MAKEWPARAM((WORD)IDUSBREFRESH, 0), 
                           (LPARAM)0, SMTO_NORMAL,
                           0, NULL);
    }
    return 0;
}

INT_PTR CALLBACK AboutDialogProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
            {
                HICON about_usb_icon = (HICON)LoadImage(GetModuleHandle(NULL), 
                                        MAKEINTRESOURCE(IDI_USB2MAILICONLARGE),
                                        IMAGE_ICON, 128, 128, 0);
                if (about_usb_icon)
                    SendDlgItemMessage(hwnd, IDUSB2MAIL, BM_SETIMAGE, 
                                      (WPARAM)IMAGE_ICON, (LPARAM)about_usb_icon);
            }
            /*{
                TCHAR tmpmsg1[255], tmpmsg2[255];
                LoadString(*g_hInst, ID_ERR_MSG_31, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                LoadString(*g_hInst, ID_ERR_MSG_30, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                SetDlgItemText(hwnd, IDC_ABOUT_BUILD, tmpmsg1);
                SetDlgItemText(hwnd, IDC_ABOUT_COMPILER, tmpmsg2);
                
            }*/
            SetDlgItemText(hwnd, IDC_ABOUT_BUILD, _T("USB2Email "U2MWin32_VERSION_STR" "WINARCH));
            SetDlgItemText(hwnd, IDC_ABOUT_COMPILER, _T("built with "COMPILER_NAME_STR" "COMPILER_VERSION_STR));
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDUSB2MAIL:
                    EndDialog(hwnd, (INT_PTR)TRUE);
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
            if (pass)
                SetDlgItemTextA(hwnd, IDC_PWDFIELD, pass);
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    ClearPwd();
                    if (parsePwdField(hwnd))
                        EndDialog(hwnd, (INT_PTR)TRUE);
                    else
                        ClearPwd();
                    return (INT_PTR)TRUE;
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
            if (SMTP_SERVER)
                SetDlgItemTextA(hwnd, IDC_SERVERURLFIELD, SMTP_SERVER);
            if (PORT) {
                char PORT_STR[9];
                snprintf(PORT_STR, 9, "%u", PORT);
                SetDlgItemTextA(hwnd, IDC_PORTFIELD, PORT_STR);
            }
            CheckDlgButton(hwnd, IDC_CHECKVALIDEMAIL, (ValidEmailCheck)?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECKUSBREFRESH, (USBRefresh)?BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, IDC_CHECKMINTOTRAY, (TrayIcon)?BST_CHECKED:BST_UNCHECKED);
            SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 50));
            SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)0);
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_SAVECONFBUTTON:
                    saveConfFile();
                    return (INT_PTR)TRUE;
                case IDAPPLY:
                case IDOK:
                    ClearPrefs();
                    if (parsePrefDialogFields(hwnd)) {
                        // i'm storing the check controls' values only if the 
                        // data entered on the input fields was valid
                        ValidEmailCheck = IsDlgButtonChecked(hwnd, IDC_CHECKVALIDEMAIL);
                        USBRefresh = IsDlgButtonChecked(hwnd, IDC_CHECKUSBREFRESH);
                        TrayIcon = IsDlgButtonChecked(hwnd, IDC_CHECKMINTOTRAY);
                        MAX_FAILED_EMAILS = (UINT)SendDlgItemMessage(hwnd, IDT_TRACKEMAILINTERVAL, 
                                                  TBM_GETPOS, (WPARAM)0, (LPARAM)0);
                        if (TrayIcon) {
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
    HANDLE refresh_usb_hnd ATTRIB_UNUSED = NULL; //usb auto scan thread handle
    static INT temp_idx = -1;

    switch (msg) {
        case WM_INITDIALOG:
            {
                HICON refreshDlgIco = (HICON)LoadImage(GetModuleHandle(NULL),
                                                 MAKEINTRESOURCE(IDI_REFRESHICON),
                                                 IMAGE_ICON, 18, 16, LR_LOADTRANSPARENT);
                if (refreshDlgIco) {
                    SendDlgItemMessage(hwnd, IDUSBREFRESH, BM_SETIMAGE,
                                      (WPARAM)IMAGE_ICON, (LPARAM)refreshDlgIco);
                }

                HICON usbDlgIco = (HICON)LoadImage(GetModuleHandle(NULL),
                                                 MAKEINTRESOURCE(IDI_USB2MAILICONMEDIUM),
                                                 IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), 
                                                 GetSystemMetrics(SM_CYSMICON), 0);
                if (usbDlgIco) {
                    SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)usbDlgIco);
                }
                
            }

            temp_idx = -1;
            vendCol.mask = devcCol.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
            vendCol.iSubItem = 0;
            devcCol.iSubItem = 1;
            vendCol.cx = devcCol.cx = 150;
            vendCol.fmt = LVCFMT_LEFT;
            devcCol.fmt = LVCFMT_RIGHT;
            {
                TCHAR tmpmsg1[255], tmpmsg2[255];
                LoadString(*g_hInst, ID_ERR_MSG_29, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                LoadString(*g_hInst, ID_ERR_MSG_28, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                devcCol.pszText = tmpmsg1;  //vendor
                vendCol.pszText = tmpmsg2;  //device
                ListView_InsertColumn(GetDlgItem(hwnd, IDC_USBDEVLIST), 0, &vendCol);
                ListView_InsertColumn(GetDlgItem(hwnd, IDC_USBDEVLIST), 1, &devcCol);
            }
            ListView_SetExtendedListViewStyle(GetDlgItem(hwnd, IDC_USBDEVLIST), LVS_EX_FULLROWSELECT);
            ListView_SetColumnWidth(GetDlgItem(hwnd, IDC_USBDEVLIST), 0, 210);
            ListView_SetColumnWidth(GetDlgItem(hwnd, IDC_USBDEVLIST), 1, 150);
            CenterChild(hwnd);
            GetConnectedUSBDevs(hwnd, FILL_USB_LISTVIEW);
            if (USBRefresh) {
                USBdev_scan = TRUE;
                refresh_usb_hnd = (HANDLE)_beginthreadex(NULL, 0, RefreshUSBThread, (LPVOID)hwnd, 0, NULL);
            }
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            usb_idx = 0;
            memset(usb_id_selection, 0, sizeof(UINT)*2);
            switch (LOWORD(wParam)) {
                case IDUSBREFRESH:
                    ListView_DeleteAllItems(GetDlgItem(hwnd, IDC_USBDEVLIST));
                    DeleteScannedUSBIDs();
                    GetConnectedUSBDevs(hwnd, FILL_USB_LISTVIEW);
                    if (temp_idx != -1)
                        ListView_SetItemState(GetDlgItem(hwnd, IDC_USBDEVLIST), (UINT)temp_idx,
                                              LVIS_FOCUSED | LVIS_SELECTED, 0x000f);
                    return (INT_PTR)TRUE;
                case IDOK:
                    if (!GetUSBListViewSelection(hwnd)) {
                        TCHAR tmpmsg1[255], tmpmsg2[255];
                        LoadString(*g_hInst, ID_ERR_MSG_27, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                        LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);
                    } else {
                        DeleteScannedUSBIDs();
                        USBdev_scan = FALSE;
                        EndDialog(hwnd, (INT_PTR)TRUE);
                    }
                    return (INT_PTR)TRUE;
                case IDCANCEL:
                    DeleteScannedUSBIDs();
                    USBdev_scan = FALSE;
                    EndDialog(hwnd, (INT_PTR)TRUE);
                    return (INT_PTR)TRUE;
            }
            return (INT_PTR)TRUE;
        case WM_NOTIFY:
            switch (LOWORD(wParam)) {
                case IDC_USBDEVLIST:
                    switch (((LPNMHDR)lParam)->code) {
                        case NM_DBLCLK:
                            usb_idx = 0;
                            if (!GetUSBListViewSelection(hwnd)) {
                                memset(usb_id_selection, 0, sizeof(usb_id_selection));
                                TCHAR tmpmsg1[255], tmpmsg2[255];
                                LoadString(*g_hInst, ID_ERR_MSG_27, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                                LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                                MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);
                            } else {
                                USBdev_scan = FALSE;
                                EndDialog(hwnd, (INT_PTR)TRUE);
                            }
                            return (INT_PTR)TRUE;                            
                        case NM_CLICK:
                            temp_idx = (INT)SendMessage(GetDlgItem(hwnd, IDC_USBDEVLIST), 
                                                        LVM_GETNEXTITEM, -1, LVNI_FOCUSED | LVNI_SELECTED);
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
            {
                HICON emailDlgIco = (HICON)LoadImage(GetModuleHandle(NULL),
                                                 MAKEINTRESOURCE(IDI_EMAILICON),
                                                 IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), 
                                                 GetSystemMetrics(SM_CYSMICON), 0);
                if (emailDlgIco) {
                    SendMessage(hwnd, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)emailDlgIco);
                }

                TCHAR tmpmsg1[255], tmpmsg2[255], tmpmsg3[255], tmpmsg4[255], tmpmsg5[255];
                LoadString(*g_hInst, ID_ERR_MSG_26, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                LoadString(*g_hInst, ID_ERR_MSG_25, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                LoadString(*g_hInst, ID_ERR_MSG_24, tmpmsg3, sizeof(tmpmsg3)/sizeof(tmpmsg3[0]));
                LoadString(*g_hInst, ID_ERR_MSG_23, tmpmsg4, sizeof(tmpmsg4)/sizeof(tmpmsg4[0]));
                LoadString(*g_hInst, ID_ERR_MSG_22, tmpmsg5, sizeof(tmpmsg5)/sizeof(tmpmsg5[0]));

                FROM_ttip = CreateBaloonToolTip(IDC_FROMFIELD, hwnd,tmpmsg1);
                TO_ttip = CreateBaloonToolTip(IDC_TOFIELD, hwnd, tmpmsg2);
                CC_ttip = CreateBaloonToolTip(IDC_CCFIELD, hwnd, tmpmsg3);
                SUBJECT_ttip = CreateBaloonToolTip(IDC_SUBJECTFIELD, hwnd, tmpmsg4);
                BODY_ttip = CreateBaloonToolTip(IDC_MESSAGEFIELD, hwnd, tmpmsg5);
            }
            if (FROM)
                SetDlgItemTextA(hwnd, IDC_FROMFIELD, FROM);
            if (TO)
                SetDlgItemTextA(hwnd, IDC_TOFIELD, TO);
            if (CC)
                SetDlgItemTextA(hwnd, IDC_CCFIELD, CC);
            if (SUBJECT)
                SetDlgItemTextA(hwnd, IDC_SUBJECTFIELD, SUBJECT);
            if (BODY)
                SetDlgItemTextA(hwnd, IDC_MESSAGEFIELD, BODY);
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    ClearEmailData();
                    if (parseEmailDialogFields(hwnd))
                        EndDialog(hwnd, (INT_PTR)TRUE);
                    else
                        ClearEmailData();
                    break;
                case IDCANCEL:
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

                TCHAR tmp1[3000];
                LoadString(*g_hInst, ID_HELP_MSG, tmp1, sizeof(tmp1)/sizeof(tmp1[0]));
                SetDlgItemText(hwnd, IDC_HELP_TEXT, tmp1);
            }
            CenterChild(hwnd);
            return (INT_PTR)TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    HlpDlg_open = FALSE;
                    EndDialog(hwnd, (INT_PTR)TRUE);
                    return (INT_PTR)TRUE;
            }
            break;
        case WM_CLOSE:
            HlpDlg_open = FALSE;
            EndDialog(hwnd, (INT_PTR)TRUE);
            return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HFONT mainwindowcontrol_font_big, mainwindowcontrol_font;

    switch (msg) {
        case WM_CREATE:
            if (TrayIcon) InitU2MTray(hwnd);
            {
                TCHAR tmp1[255], tmp2[255], tmp3[255], tmp4[255];
                LoadString(*g_hInst, ID_ERR_MSG_21, tmp1, sizeof(tmp1)/sizeof(tmp1[0]));
                LoadString(*g_hInst, ID_ERR_MSG_19, tmp2, sizeof(tmp2)/sizeof(tmp2[0]));
                LoadString(*g_hInst, ID_ERR_MSG_9, tmp3, sizeof(tmp3)/sizeof(tmp3[0]));
                LoadString(*g_hInst, ID_ERR_MSG_15, tmp4, sizeof(tmp4)/sizeof(tmp4[0]));

                mainwindowcontrol_font_big = CreateFont(
                                24, 0, //height, width
                                0, 0, //escapement, orientation angles
                                FW_DONTCARE, //boldness 0-1000, 400 = normal, 700 = bold
                                FALSE, FALSE, FALSE, //italics, underline, strikeout
                                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, //charset, output precision
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, //clipping, output quality
                                VARIABLE_PITCH, _T("Trebuchet MS"));

                mainwindowcontrol_font = CreateFont(18, 0, 0, 0, 550,
                                FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, 
                                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                VARIABLE_PITCH, _T("Trebuchet MS"));

                USBListButton = CreateWindowEx(0, _T("BUTTON"), tmp1,
                                 WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                 30, 30, 200, 30, hwnd, (HMENU)IDC_CHOOSEUSBBUTTON,
                                 (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
                if (!USBListButton) {
                    TCHAR tmpmsg1[255], tmpmsg2[255];
                    LoadString(*g_hInst, ID_ERR_MSG_20, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                    LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                    MessageBoxEx(NULL, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);
                    return -2;
                }
                SendMessage(USBListButton, WM_SETFONT, (WPARAM)mainwindowcontrol_font, (LPARAM)TRUE);

                EMAILButton = CreateWindowEx(0, _T("BUTTON"), tmp2,
                                 WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                                 300, 30, 200, 30, hwnd, (HMENU)IDC_EMAILBUTTON, 
                                 (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
                if (!EMAILButton) {
                    TCHAR tmpmsg1[255], tmpmsg2[255];
                    LoadString(*g_hInst, ID_ERR_MSG_18, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                    LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                    MessageBoxEx(NULL, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);
                    return -2;
                }
                SendMessage(EMAILButton, WM_SETFONT, (WPARAM)mainwindowcontrol_font, (LPARAM)TRUE);

                STARTSTOP = CreateWindowEx(0, _T("BUTTON"), tmp3,
                                 WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                                 300, 200, 200, 50, hwnd, (HMENU)IDC_STARTSTOP, 
                                 *g_hInst, NULL);
                if (!STARTSTOP) {
                    TCHAR tmpmsg1[255], tmpmsg2[255];
                    LoadString(*g_hInst, ID_ERR_MSG_17, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                    LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                    MessageBoxEx(NULL, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);
                    return -2;
                }
                SendMessage(STARTSTOP, WM_SETFONT, (WPARAM)mainwindowcontrol_font_big, (LPARAM)TRUE);

                time_track = CreateWindowEx(0, TRACKBAR_CLASS, _T(""),
                                 WS_CHILD | WS_VISIBLE | TBS_TOOLTIPS | TBS_NOTICKS | TBS_HORZ,
                                 30, 200, 200, 30, hwnd, (HMENU)IDC_TIMETRACK,
                                 *g_hInst, NULL);
                if (!time_track) {
                    TCHAR tmpmsg1[255], tmpmsg2[255];
                    LoadString(*g_hInst, ID_ERR_MSG_16, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                    LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                    MessageBoxEx(NULL, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);
                    return -2;
                }

                ttrack_label = CreateWindow(_T("STATIC"), tmp4,
                                 WS_VISIBLE | WS_CHILD | SS_CENTER,
                                 30, 160, 200, 40, hwnd, NULL, *g_hInst, NULL);
                if (!ttrack_label) {
                    TCHAR tmpmsg1[255], tmpmsg2[255];
                    LoadString(*g_hInst, ID_ERR_MSG_14, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                    LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                    MessageBoxEx(NULL, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);
                    return -2;
                }
                SendMessage(ttrack_label, WM_SETFONT, (WPARAM)mainwindowcontrol_font, (LPARAM)TRUE);

                SendMessage(time_track, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ttrack_struct);
                SendMessage(time_track, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(T_MIN, T_MAX));
                SendMessage(time_track, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)4);
                SendMessage(time_track, TBM_SETSEL, (WPARAM)FALSE, (LPARAM)MAKELONG(T_MIN, T_MAX));
                SendMessage(time_track, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)1000);
                UpdateWindow(hwnd);
            }
        case WM_MOUSEMOVE:
            break;
        case WM_PAINT:
            if ((hdc = BeginPaint(hwnd, &ps))) {
                EndPaint(hwnd, &ps);
            }
            break;
        /* custom message to use from the U2MThread */
        case WM_ENABLE_STARTSTOP:
            EnableWindow(STARTSTOP, !IsWindowEnabled(STARTSTOP));
            SetU2MNotifyTip();
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_EN_LANG:
                        *g_hInst = U2M_dlls.U2MLocale_en.module;
                        currentLangID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
                        ResetMainWindowLanguage(hwnd);
                        MainMenu = U2M_dlls.U2MLocale_en.locale_menu;
                        SetMenu(hwnd, MainMenu);
                        SetU2MNotifyTip();
                    break;
                case IDM_GR_LANG:
                    *g_hInst = U2M_dlls.U2MLocale_gr.module;
                    currentLangID = MAKELANGID(LANG_GREEK, SUBLANG_GREEK_GREECE);
                    ResetMainWindowLanguage(hwnd);
                    MainMenu = U2M_dlls.U2MLocale_gr.locale_menu;
                    SetMenu(hwnd, MainMenu);
                    SetU2MNotifyTip();
                    break;
                case IDM_ABOUT:
                    InitAboutDialog(hwnd);
                    break;
                case IDM_PREF:
                    if (!onoff)
                        InitPreferencesDialog(hwnd);
                    else {
                        TCHAR tmpmsg1[255], tmpmsg2[255];
                        LoadString(*g_hInst, ID_ERR_MSG_13, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                        LoadString(*g_hInst, ID_ERR_MSG_1, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_ICONEXCLAMATION | MB_OK, currentLangID);
                    }
                    break;
                case IDM_PASSWORD:
                    if (!onoff)
                        InitPasswordDialog(hwnd);
                    else {
                        TCHAR tmpmsg1[255], tmpmsg2[255];
                        LoadString(*g_hInst, ID_ERR_MSG_12, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                        LoadString(*g_hInst, ID_ERR_MSG_1, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_ICONEXCLAMATION | MB_OK, currentLangID);
                    }
                    break;
                case IDC_CHOOSEUSBBUTTON:
                    if (!onoff)
                        InitUSBDialog(hwnd);
                    else {
                        TCHAR tmpmsg1[255], tmpmsg2[255];
                        LoadString(*g_hInst, ID_ERR_MSG_11, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                        LoadString(*g_hInst, ID_ERR_MSG_1, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_ICONEXCLAMATION | MB_OK, currentLangID);
                    }
                    break;
                case IDC_STARTSTOP:
                    if (onoff) {
                        EnableWindow(USBListButton, TRUE);
                        EnableWindow(EMAILButton, TRUE);
                        EnableWindow(time_track, TRUE);
                        EnableWindow(ttrack_label, TRUE);
                        onoff = FALSE;
                    } else {
                        if (InitU2MThread(hwnd)) {
                            EnableWindow(USBListButton, FALSE);
                            EnableWindow(EMAILButton, FALSE);
                            EnableWindow(time_track, FALSE);
                            EnableWindow(ttrack_label, FALSE);
                        }
                    }
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
                    else {
                        TCHAR tmpmsg1[255], tmpmsg2[255];
                        LoadString(*g_hInst, ID_ERR_MSG_8, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                        LoadString(*g_hInst, ID_ERR_MSG_1, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                        MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_ICONEXCLAMATION | MB_OK, currentLangID);
                    }
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
                    if (TrayIcon) {
                        ShowWindow(hwnd, SW_HIDE);
                    } else {
                        ShowWindow(hwnd, SW_MINIMIZE);
                    }
                    break;
                case SC_CLOSE:
                    PostMessage(hwnd, WM_CLOSE, wParam, lParam);
                    break;
                default:
                    return DefWindowProc(hwnd, msg, wParam, lParam);
            }
            break;
        case WM_U2M_NOTIF_ICON:
            switch (lParam) {
                case WM_RBUTTONUP:
                    {
                        POINT cursor_pos;
                        TCHAR tmpmsg1[255], tmpmsg2[255];

                        LoadString(*g_hInst, ID_ERR_MSG_60, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                        LoadString(*g_hInst, ID_ERR_MSG_61, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                        TrayIconMenu = CreatePopupMenu();
                        AppendMenu(TrayIconMenu, MF_STRING, IDM_TRAY_OPENWINDOW, tmpmsg2);
                        AppendMenu(TrayIconMenu, MF_STRING | MF_SEPARATOR, 0, NULL);
                        AppendMenu(TrayIconMenu, MF_STRING, IDM_TRAY_QUIT, tmpmsg1);

                        GetCursorPos(&cursor_pos);
                        SetForegroundWindow(hwnd);
                        if (!TrackPopupMenuEx(TrayIconMenu,
                                 TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION,
                                 cursor_pos.x, cursor_pos.y, hwnd, NULL)) printf("TRACKPOPUPMENUEX() FAILED\n");
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
                TCHAR tmpmsg1[255], tmpmsg2[255];
                LoadString(*g_hInst, ID_ERR_MSG_7, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                LoadString(*g_hInst, ID_ERR_MSG_1, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_ICONEXCLAMATION | MB_OK, currentLangID);
                break;
            }
#ifndef DEBUG
            {
                TCHAR tmpmsg1[255], tmpmsg2[255];
                LoadString(*g_hInst, ID_ERR_MSG_6, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
                LoadString(*g_hInst, ID_ERR_MSG_5, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
                if (MessageBoxEx(hwnd, tmpmsg1, tmpmsg2, MB_ICONASTERISK | MB_YESNO, currentLangID) == IDNO)
                    break;
            }
#endif
            DeleteAll();
            DeleteObject(mainwindowcontrol_font_big);
            DeleteObject(mainwindowcontrol_font);
            FreeLibrary(U2M_dlls.U2MLocale_gr.module);
            FreeLibrary(U2M_dlls.U2MLocale_en.module);
            WriteDataToU2MReg();
            DestroyWindow(hwnd);
            break;
        case WM_HSCROLL:
            TIMEOUT = (UINT)SendMessage(time_track, TBM_GETPOS, (WPARAM)0, (LPARAM)0);
            break;
        case WM_DESTROY:
            DeleteAll();
            DeleteObject(mainwindowcontrol_font_big);
            DeleteObject(mainwindowcontrol_font);
            FreeLibrary(U2M_dlls.U2MLocale_gr.module);
            FreeLibrary(U2M_dlls.U2MLocale_en.module);
            DestroyWindow(hwnd);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    HWND hwnd;
    MSG Msg;
    BOOL bRet = TRUE;
    RECT wrkspace_px;
    INT center_x, center_y;
    INITCOMMONCONTROLSEX columnControlClass = {sizeof(INITCOMMONCONTROLSEX), ICC_LISTVIEW_CLASSES};


    /*** Global initializations ***/
    PORT = 0;
    pass = CC = TO = FROM = SUBJECT = BODY = SMTP_SERVER = NULL;
    TrayIconMenu = NULL;
    memset(usb_id_selection, 0, sizeof(UINT)*2);
    TIMEOUT = 1000;

    GetU2MRegData();

    g_hInst = &hInstance; //possibly a bad decision

    /* loading the language dlls */
    U2M_dlls.U2MLocale_gr.module = LoadLibraryEx(U2M_dlls.U2MLocale_gr.filename, NULL, 0);
    U2M_dlls.U2MLocale_en.module = LoadLibraryEx(U2M_dlls.U2MLocale_en.filename, NULL, 0);

    U2M_dlls.U2MLocale_en.locale_menu = LoadMenu(U2M_dlls.U2MLocale_en.module, MAKEINTRESOURCE(IDR_MAINMENU));
    U2M_dlls.U2MLocale_gr.locale_menu = LoadMenu(U2M_dlls.U2MLocale_gr.module, MAKEINTRESOURCE(IDR_MAINMENU));

    if (!U2M_dlls.U2MLocale_en.module || 
        !U2M_dlls.U2MLocale_gr.module) {
        if (MessageBox(NULL, 
                       _T("One or more language packages were not found. ")
                       _T("The application might malfunction. ")
                       _T("Are you sure you want to continue?"),
                       _T("Failed loading localization libraries!"), 
                       MB_ICONASTERISK | MB_YESNO) == IDNO)
            return 1;
    }

    currentLangID = GetUserDefaultUILanguage(); //get the language of the system
    switch (PRIMARYLANGID(currentLangID)) {
        case LANG_GREEK:
            *g_hInst = U2M_dlls.U2MLocale_gr.module;
            break;
        case LANG_ENGLISH:
        default:
            currentLangID = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            *g_hInst = U2M_dlls.U2MLocale_en.module;
            break;
    }

    InitCommonControlsEx(&columnControlClass);
    usb_idx = 0;

    parseConfFile();

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
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_4, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(NULL, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);
        return -1;
    }

    hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, szClassName, _T("USB2Email"),
                          WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_EX_LAYERED,
                          center_x, center_y, 550, 350, NULL, NULL, *g_hInst, NULL);

    if (!hwnd) {
        TCHAR tmpmsg1[255], tmpmsg2[255];
        LoadString(*g_hInst, ID_ERR_MSG_3, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
        LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
        MessageBoxEx(NULL, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);
        return -2;
    }

    InitCommonControls();

#ifdef DEBUG
    ShowWindow(hwnd, SW_SHOW);
    SetActiveWindow(hwnd);
#else //GDI usage errors with AnimateWindow!?
    AnimateWindow(hwnd, 200, AW_CENTER | AW_ACTIVATE);
    //PrintWindow(hwnd, GetWindowDC(hwnd), 0);
#endif
    while (bRet) {
        bRet = GetMessage(&Msg, NULL, 0, 0);

        if (bRet == -1) {
            TCHAR tmpmsg1[255], tmpmsg2[255];
            LoadString(*g_hInst, ID_ERR_MSG_2, tmpmsg1, sizeof(tmpmsg1)/sizeof(tmpmsg1[0]));
            LoadString(*g_hInst, ID_ERR_MSG_0, tmpmsg2, sizeof(tmpmsg2)/sizeof(tmpmsg2[0]));
            MessageBoxEx(NULL, tmpmsg1, tmpmsg2, MB_ICONERROR | MB_OK, currentLangID);
            return -3;
        } else if (bRet == -2) {
            return -2;
        }

        UpdateWindow(hwnd);
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
}
